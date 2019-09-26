#include "tokenlock.hpp"
#include <eosio/eosio.hpp>
#include <stdio.h>
#include <ctime>
#include <time.h>

using namespace eosio;
using namespace std;
using namespace eosio::internal_use_do_not_use;

using eosio::print;

void tokenlock::transferlock(name from,
                  name to,
                  asset quantity,
                  string memo,
                  uint64_t unlock_timestamp) {

  eosio_assert( from != to, "cannot transfer to self" );
  require_auth( from );
  eosio_assert( is_account( to ), "to account does not exist");

  require_recipient( from );
  require_recipient( to );

  eosio_assert( quantity.is_valid(), "invalid symbol" );
  eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
  eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );
  eosio_assert( unlock_timestamp > current_time_point().sec_since_epoch(), "unlock_timestamp must be future" );
  
  action(
     permission_level{from, "active"_n},
     TOKEN_CONTRACT, "transfer"_n,
     std::make_tuple(from, _self, quantity, std::string("Token locked"))
   ).send();               

  uint64_t length = std::distance(_lockups.cbegin(), _lockups.cend());
  _lockups.emplace(_self, [&](lockup &l) {
    l.no = length + 1;
    l.sender = from;
    l.receiver = to;
    l.token = quantity;
    l.memo = memo;
    l.lock_begin = current_time_point().sec_since_epoch();
    //l.lock_end = current_time_point().sec_since_epoch() + lock_days * 86400; // day in milliseconds
    l.lock_end = unlock_timestamp;
    l.tx_id_lockup = get_trx_id();
  });
}
  
/*
void tokenlock::unlock(name user, name receiver) {
  require_auth( user );
  auto iterator = _lockups.begin();
  do {
      if ((*iterator).receiver == receiver)
          break;
  } while (++iterator != _lockups.end());
  
  eosio_assert(iterator != _lockups.end(), "The receiver is not exist in lock list");
  eosio_assert(iterator->lock_end > current_time_point().sec_since_epoch(), "The locking has already been ended");

  action(
     permission_level{_self, "active"_n},
     TOKEN_CONTRACT, "transfer"_n,
     std::make_tuple(_self, receiver, iterator->token, std::string("Token unlocked"))
   ).send();
   
  _lockups.erase(iterator);
}
*/

void tokenlock::currenttime() {
  print("current time: ", current_time_point().sec_since_epoch());
}

void tokenlock::claim(name receiver, uint64_t no) {
  require_auth( receiver );
  auto iterator = _lockups.begin();
  do {
    if ((*iterator).no == no &&
        (*iterator).receiver == receiver &&
        (*iterator).lock_end < current_time_point().sec_since_epoch()) {
      action(
        permission_level{_self, "active"_n},
        TOKEN_CONTRACT, "transfer"_n,
        std::make_tuple(_self, receiver, iterator->token, std::string("Token claimed"))
      ).send();

      _lockups.modify(iterator, _self, [&](auto& row) {
        row.claim = 1;
        row.tx_id_claim = get_trx_id();
      });
      
      break;
    }
  } while (++iterator != _lockups.end());
}

void tokenlock::claimall(name receiver) {
  require_auth( receiver );
  auto iterator = _lockups.begin();
  do {
    if ((*iterator).receiver == receiver &&
        (*iterator).lock_end < current_time_point().sec_since_epoch()) {
      action(
        permission_level{_self, "active"_n},
        TOKEN_CONTRACT, "transfer"_n,
        std::make_tuple(_self, receiver, iterator->token, std::string("Token claimed"))
      ).send();

      _lockups.modify(iterator, _self, [&](auto& row) {
        row.claim = 1;
        row.tx_id_claim = get_trx_id();
      });
    }
  } while (++iterator != _lockups.end());
}

checksum256 tokenlock::get_trx_id() {
    size_t size = transaction_size();
    char buf[size];
    size_t read = read_transaction( buf, size );
    check( size == read, "read_transaction failed");
    return sha256( buf, read );
}

EOSIO_DISPATCH( tokenlock, (transferlock)/*(unlock)*/(claim)(claimall)(currenttime) )
