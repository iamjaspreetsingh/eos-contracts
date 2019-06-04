/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <crowdsale/crowdsale.hpp>

namespace eosio {

void sale::start( name recipient,
                    asset  goal )
{
    require_auth( _self );

    auto sym = goal.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    auto sy= eosio::symbol("EOS", 4);
    eosio_assert( sy==sym, "goal must be in EOS" );
    eosio_assert( is_account( recipient ), "recipient account does not exist");

    eosio_assert( goal.is_valid(), "invalid goal");
    eosio_assert( goal.amount > 0, "goal must be positive");

    stats statstable( _self, sym.code().raw() );
  //  auto existing = statstable.find( sym.code().raw() );
   // eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = goal.symbol;
       s.goal = goal;
       s.recipient = recipient;
    });
}


void sale::contribute( name from, asset quantity )
{
    require_auth( from );

    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    auto sy= eosio::symbol("EOS", 4);

    eosio_assert( sy==sym, "quantity must be in EOS" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), "EOS symbol does not exist" );
    const auto& st = *existing;

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, " quantity must be  positive" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol EOS precision mismatch" );
    eosio_assert( quantity.amount >= st.goal.amount - st.supply.amount, "quantity exceeds the goal");

   //check if recipient doesnot contribute if required
    eosio_assert( st.recipient!=from, "recipient cannot contribute" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.recipient, quantity, st.recipient );
    sub_balance( from, quantity );

}

// void sale::checkgoal(asset sym)
// {
//    //  stats statstable( _self, sym );
//    //  const auto& st = statstable.get( sym );
 
//    if(st.supply<st.goal)
//       print("Goal not Reached");
//    else    
//       print("Goal Reached");
 
// }

void sale::sub_balance( name owner, asset value ) {
   accounts from_acnts( _self, owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
         a.balance -= value;
      });
}

void sale::add_balance( name owner, asset value, name ram_payer )
{
   accounts to_acnts( _self, owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}




} /// namespace eosio

EOSIO_DISPATCH( eosio::sale, (start)(contribute))
