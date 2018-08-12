#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/transaction.hpp>

using namespace eosio;
using namespace std;

class eosdelay : public eosio::contract {
public:
    eosdelay(account_name self):eosio::contract(self), _global(_self, _self){
        auto gl_itr = _global.begin();
        if (gl_itr == _global.end())
        {
            gl_itr = _global.emplace(_self, [&](auto &gl) {
                gl.id = 0;
                gl.next_id = 0x0fffffffffffffff;
                gl.owner = _self;
            });
        }
    }

    /// @abi action 
    void delay(uint64_t ok, account_name to, asset quant, string memo){
        auto gl_itr = _global.begin();
        eosio_assert(gl_itr != _global.end(), "owner not defined");
        require_auth(gl_itr->owner);
        if(now() < ok - 3){
            transaction out; //构造交易
            out.actions.emplace_back(
                permission_level{_self, N(active)},
                _self, N(delay),
                make_tuple(ok, to, quant, string("delay"))); //将指定行为绑定到该交易上
            out.delay_sec = ok - now() - 3; //设置延迟时间，单位为1秒
            out.send(_next_id(), _self, false); //发送交易，第一个参数为该次交易发送id，每次需不同。如果两个发送id相同，则视第三个参数replace_existing来定是覆盖还是直接失败。
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
                out.send(_next_id(), _self, false); 
            }
        } else {
            eosio_assert(false, "over due");
        }
    }

    /// @abi action 
    void clear(){
        auto gl_itr = _global.begin();
        require_auth(gl_itr->owner);
        while (true) {
            gl_itr = _global.begin();
            if (gl_itr == _global.end()) {
                break;
            }
            _global.erase(gl_itr);
        }
    }

    uint64_t _next_id(){
        auto gl_itr = _global.begin();
        _global.modify(gl_itr, 0, [&](auto &gl) {
            gl.next_id++;
        });
        return gl_itr->next_id;
    }


private:
    // @abi table games i64
    struct global{
        uint64_t id;
        uint64_t next_id;
        account_name owner;
        uint64_t primary_key() const { return id; }
        EOSLIB_SERIALIZE(global, (id)(next_id)(owner))
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
       if(code == self && (action==N(delay) || action==N(clear) ||action == N(onerror))) { \
          TYPE thiscontract( self ); \
          switch( action ) { \
             EOSIO_API( TYPE, MEMBERS ) \
          } \
       } \
    } \
 }

EOSIO_ABI_EX(eosdelay, (delay)(clear))