#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>

using namespace eosio;
using namespace std;

class eosdelay : public eosio::contract {
public:
    eosdelay(account_name self):eosio::contract(self), _global(_self, _self){
        auto gl_itr = _global.begin();
        if (gl_itr == _global.end())
        {
            gl_itr = _global.emplace(_self, [&](auto& gl) {
                gl.owner = _self;
            });
        }
    }

    /// @abi action 
    void delay(uint32_t ok, account_name to, asset quant, string memo){
        auto gl_itr = _global.begin();
        eosio_assert(gl_itr != _global.end(), "owner not defined");
        require_auth(gl_itr->owner);
        if(now() < ok - 2){
            transaction out; //构造交易
            out.actions.emplace_back(
                permission_level{_self, N(active)},
                _self, N(delay),
                make_tuple(ok, to, quant, string("delay"))); //将指定行为绑定到该交易上
            out.delay_sec = ok - now() - 2; //设置延迟时间，单位为1秒
            out.send(0xffffffffffffffff, _self, true); //发送交易，第一个参数为该次交易发送id，每次需不同。如果两个发送id相同，则视第三个参数replace_existing来定是覆盖还是直接失败。
        } else if(memo == "delay"){
            if(now() >= ok){
                action(
                    permission_level{_self, N(active)},
                    N(eosio.token), N(transfer),
                    make_tuple(_self, to, quant, memo))
                .send();
            } else {
                transaction out; //构造交易
                out.actions.emplace_back(
                    permission_level{_self, N(active)},
                    _self, N(delay),
                    make_tuple(ok, to, quant, string("delay")));
                out.delay_sec = 1;
                out.send(0xffffffffffffffff, _self, true); 
            }
        } else {
            eosio_assert(false, "over due");
        }
    }

private:
    // @abi table global i64
    struct global{
        account_name owner;
        uint64_t primary_key() const { return owner; }
        EOSLIB_SERIALIZE(global, (owner))
    };

    typedef eosio::multi_index<N(global), global> global_index;
    global_index _global;
};

 #define EOSIO_ABI_EX( TYPE, MEMBERS ) \
 extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
       if( action == N(onerror)) { \
          eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account"); \
       } \
       auto self = receiver; \
       if(code == self && (action==N(delay) || action == N(onerror))) { \
          TYPE thiscontract( self ); \
          switch( action ) { \
             EOSIO_API( TYPE, MEMBERS ) \
          } \
       } \
    } \
 }

EOSIO_ABI_EX(eosdelay, (delay))