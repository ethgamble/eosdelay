#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/transaction.hpp>

using namespace eosio;
using namespace std;

class eosdelay : public eosio::contract {
public:
    eosdelay(account_name self):
    eosio::contract(self), 
    _global(_self, _self)
    {
        auto gl_itr = _global.begin();
        if (gl_itr == _global.end())
        {
            gl_itr = _global.emplace(_self, [&](auto& gl) {
                gl.owner = _self;
                gl.next_id = 0x0ffffffffffffff0;
            });
        }
    }

    /// @abi action 
    void delay(uint32_t due, account_name from, account_name to, asset quant, string memo){
        auto gl_itr = _global.begin();
        eosio_assert(gl_itr != _global.end(), "owner not defined");
        require_auth(gl_itr->owner);
        if(now() < due - 2){
            // To notifiy the transfer
            action(
                permission_level{from, N(active)},
                N(eosio.token), N(transfer),
                make_tuple(from, _self, quant, string("delay ") + int2str(due) + string(" now ") + int2str(now()) + string(" remain ") + int2str(due - now())))
            .send();
            transaction out; //构造交易
            out.actions.emplace_back(
                permission_level{_self, N(active)},
                _self, N(delay),
                make_tuple(due, from, to, quant, string("delay ") + int2str(due) + string(" now ") + int2str(now()) + string(" remain ") + int2str(due - now()))); //将指定行为绑定到该交易上
            //设置延迟时间，单位为1秒
            if((due - now()) / 2 <= 1){
                out.delay_sec = due - now() - 1;
            } else {
                out.delay_sec = (due - now()) / 2;
            }
            out.send(_next_id(), _self, true); //发送交易，第一个参数为该次交易发送id，每次需不同。如果两个发送id相同，则视第三个参数replace_existing来定是覆盖还是直接失败。
        } else if(startWith(memo, "delay")){
            // if(current_time() >= due * 1000000ll || current_time() > (due * 1000000ll - 500000ll)){
            if(current_time() >= due * 1000000ll){
                action(
                    permission_level{from, N(active)},
                    N(eosio.token), N(transfer),
                    make_tuple(from, to, quant, string("")))
                .send();
                // To notifiy the transfer has been sent
                action(
                    permission_level{from, N(active)},
                    N(eosio.token), N(transfer),
                    make_tuple(from, _self, quant, string("delay ") + int2str(due) + string(" excute ") + int2str(now())))
                .send();
            } else {
                transaction out; //构造交易
                out.actions.emplace_back(
                    permission_level{_self, N(active)},
                    _self, N(delay),
                    make_tuple(due, from, to, quant, string("delay ") + int2str(due) + string(" now ") + int2str(now()) + string(" remain ") + int2str(due - now())));
                out.delay_sec = 1;
                out.send(_next_id(), _self, true); 
            }
        } else {
            eosio_assert(false, "over due");
        }
    }

    uint64_t _next_id(){
        auto gl_itr = _global.begin();
        _global.modify(gl_itr, 0, [&](auto& gl){
            gl.next_id++;
        });
        return gl_itr->next_id;
    }

private:
    string int2str(uint64_t x){
        string tmp(""), ans("");
        while(x > 0)
        {
            tmp += (x % 10) + 48;
            x /= 10;
        }
        for(int i = tmp.size() - 1; i >= 0; i--)
        {
            ans += tmp[i];
        }
        return ans;
    }  

    bool startWith(const string &str, const string &head) {
	    return str.compare(0, head.size(), head) == 0;
    }

    // @abi table global i64
    struct global{
        account_name owner;
        uint64_t next_id;
        uint64_t primary_key() const { return owner; }
        EOSLIB_SERIALIZE(global, (owner)(next_id))
    };

    typedef eosio::multi_index<N(global), global> global_index;
    global_index _global;

    // TODO add other contract's struct to inspect data



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
