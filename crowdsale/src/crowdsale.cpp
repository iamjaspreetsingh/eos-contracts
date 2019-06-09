/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <crowdsale/crowdsale.hpp>
// #include "src/eosio.token.cpp"
const uint32_t seconds_in_one_day = 60 * 60 * 24;
const uint32_t seconds_in_one_week = seconds_in_one_day * 7;
const uint32_t seconds_in_one_year = seconds_in_one_day * 365;
const uint32_t _rate = 1;

namespace eosio {

void sale::start(uint64_t compaignID, name recipient,
                    asset  goal )
{
    require_auth(_self);

    auto sym = goal.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    auto sy= eosio::symbol("EOS", 4);
    eosio_assert( sy==sym, "goal must be in EOS" );
    eosio_assert( is_account( recipient ), "recipient account does not exist");

    eosio_assert( goal.is_valid(), "invalid goal");
    eosio_assert( goal.amount > 0, "goal must be positive");

    stats statstable( _self, compaignID);
  //  auto existing = statstable.find( sym.code().raw() );
   // eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.compaignID = compaignID; 
       s.supply.symbol = goal.symbol;
       s.goal.symbol = goal.symbol;
       s.goal = goal;
       s.state=0;
       s.recipient = recipient;
       s.end_date = time_point_sec(now() + 360);

    });
}


void sale::contribute(uint64_t compaignID, name from, asset quantity )
{
    require_auth(from);

    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    auto sy= eosio::symbol("EOS", 4);

    eosio_assert( sy==sym, "quantity must be in EOS" );

    stats statstable( _self, compaignID );
    auto existing = statstable.find( compaignID );
    eosio_assert( existing != statstable.end(), "Compaign ID does not exist" );
    const auto& st = *existing;

    eosio_assert( st.state==0, "Crowdsale is paused/ finished" );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, " quantity must be  positive" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol EOS precision mismatch" );
    eosio_assert( quantity.amount <= st.goal.amount - st.supply.amount, "quantity exceeds the goal");

   //check if recipient doesnot contribute if required
    eosio_assert( st.recipient!=from, "recipient cannot contribute" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

   // transferTokens(from, get_self(), quantity, "crowdsaleTranfer");

    //add_balance( st.recipient, quantity, st.recipient );
    //sub_balance( from, quantity );

}


void sale::pause(uint64_t compaignID)
{
    require_auth(_self);
    stats statstable( _self, compaignID );
    auto existing = statstable.find( compaignID );
    eosio_assert( existing != statstable.end(), "Compaign ID does not exist" );
    const auto& st = *existing;
    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.state = 1;
    });

}

void sale::stop(uint64_t compaignID)
{
    require_auth(_self);
    stats statstable( _self, compaignID );
    auto existing = statstable.find( compaignID );
    eosio_assert( existing != statstable.end(), "Compaign ID does not exist" );
    const auto& st = *existing;
    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.state = 2;
    });
}

void sale::rate(uint64_t compaignID)
{
    eosio::print ("Rate of 1 token:");
    eosio::print (_rate);
}

void sale::checkgoal(uint64_t compaignID)
{
    stats statstable( _self, compaignID );
    auto existing = statstable.find( compaignID );
    eosio_assert( existing != statstable.end(), "Compaign ID does not exist" );
    const auto& st = *existing;
   if(st.supply<st.goal)
    {
        print("Goal not Reached:");
        print(st.goal);
    }
   else    
      print("Goal Reached");

}



}
EOSIO_DISPATCH(eosio::sale, (start)(contribute)(pause)(stop)(rate)(checkgoal))


