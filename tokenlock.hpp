#include <eosio/eosio.hpp>
#include <eosio/symbol.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>

using namespace eosio;
using namespace std;

class [[eosio::contract]] tokenlock : public contract {
  public:
    tokenlock(name receiver, name code, datastream<const char*> ds )
        : contract(receiver, code, ds),
        _lockups(receiver, code.value) {}
        
    const name TOKEN_CONTRACT = "osb.token"_n;
    
    [[eosio::action]] void transferlock(name from,
                      name to,
                      asset quantity,
                      string memo,
                      uint64_t lock_days);
    [[eosio::action]] void claim(name receiver);
    [[eosio::action]] void currenttime();
    
  private:
    struct [[eosio::table]] lockup {
        uint64_t no;
        name sender;
        name receiver;
        asset token;
        string memo;
        uint32_t lock_begin = 0;
        uint32_t lock_end = 0;
        uint64_t primary_key() const { return no; }
    };
    
    typedef multi_index<"lockup"_n, lockup> lockup_table;

    lockup_table _lockups;
};