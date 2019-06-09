/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class [[eosio::contract("crowdsale")]] sale : public contract {
      public:
         using contract::contract;
         
         sale(name receiver, name code,  datastream<const char*> ds): contract(receiver, code, ds) {}

         [[eosio::action]]
         void start(uint64_t compaignID,  name   recipient,
                      asset  goal);

         [[eosio::action]]
         void contribute(uint64_t compaignID, name from, asset quantity );

         [[eosio::action]]
         void pause(uint64_t compaignID );

         [[eosio::action]]
         void stop(uint64_t compaignID);


       //  void transfer(name from, name to, asset quantity, string memo);

         [[eosio::action]]
         void rate(uint64_t compaignID);

         [[eosio::action]]
          void checkgoal(uint64_t compaignID);


/////////////////////////////////////////////////
         // [[eosio::action]]
         // void getContributionAmount();

         // [[eosio::action]]
         // void open( name owner, const symbol& symbol, name ram_payer );

         static asset get_( name token_contract_account, symbol_code sym_code )
         {
            stats statstable( token_contract_account, sym_code.raw() );
            const auto& st = statstable.get( sym_code.raw() );
            return st.supply;
         }

         static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.balance;
         }

      private:
         struct [[eosio::table]] account {
            asset    balance;
            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         struct [[eosio::table]] salestats {
            uint64_t compaignID;
            asset    supply;
            asset    goal;
            uint64_t state;
            name     recipient;
            time_point_sec end_date;

            uint64_t primary_key()const { return compaignID; }
         };

         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, salestats > stats;

         void sub_balance( name owner, asset value );
         void add_balance( name owner, asset value, name ram_payer );
         void transferTokens(name from, name to, asset quantity, string memo);
 
   };

} /// namespace eosio
