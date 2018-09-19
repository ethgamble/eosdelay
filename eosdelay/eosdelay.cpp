#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
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

    __attribute__((eosio_action)) 
    void delay(uint32_t due, account_name from, account_name to, asset quant, string memo){
        auto gl_itr = _global.begin();
        eosio_assert(gl_itr != _global.end(), "owner not defined");
        require_auth(gl_itr->owner);
        if( now() > due + 2 ) return;
        if( now() < due - 2 ){
            transaction out; //构造交易
            out.actions.emplace_back(
                permission_level{_self, N(active)},
                _self, N(delay),
                make_tuple(due, from, to, quant, string("delay"))); //将指定行为绑定到该交易上
            //设置延迟时间，单位为1秒
            if((due - now()) / 2 <= 1){
                out.delay_sec = due - now() - 1;
            } else {
                out.delay_sec = (due - now()) / 2;
            }
            out.send(_next_id(), _self, true); //发送交易，第一个参数为该次交易发送id，每次需不同。如果两个发送id相同，则视第三个参数replace_existing来定是覆盖还是直接失败。
        } else if(startWith(memo, "delay")){
            transaction out; 
            out.actions.emplace_back(
                    permission_level{_self, N(active)},
                    _self, N(delay),
                    make_tuple(due, from, to, quant, string("delay")));
            out.delay_sec = 0;
            out.send(_next_id(), _self, true); 
            
            transaction out1; 
            out1.actions.emplace_back(
                    permission_level{_self, N(active)},
                    _self, N(delay),
                    make_tuple(due, from, to, quant, string("action")));
            out1.delay_sec = 0;
            out1.send(_next_id(), _self, true); 
            
        } else if(startWith(memo, "action")){
            action(
                permission_level{from, N(active)},
                N(eosio.token), N(transfer),
                make_tuple(from, to, quant, string("")))
            .send();
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

    bool startWith(const string &str, const string &head) {
	    return str.compare(0, head.size(), head) == 0;
    }

    struct __attribute__((eosio_table)) global{
        account_name owner;
        uint64_t next_id;
        uint64_t primary_key() const { return owner; }
        EOSLIB_SERIALIZE(global, (owner)(next_id))
    };

    typedef eosio::multi_index<N(global), global> global_index;
    global_index _global;

    // TODO add other contract's struct to inspect data
    
};

#undef EOSIO_ABI
#define EOSIO_ABI( TYPE, MEMBERS ) \
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

EOSIO_ABI(eosdelay, (delay))