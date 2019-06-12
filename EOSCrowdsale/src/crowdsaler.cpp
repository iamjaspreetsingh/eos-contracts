#include "crowdsaler.hpp"

#include "config.h"

const eosio::symbol sy_eos = eosio::symbol("EOS", 4);

#define NOW now() // rename system func that returns time in seconds

// constructor
crowdsaler::crowdsaler(eosio::name self, eosio::name code, eosio::datastream<const char *> ds) : eosio::contract(self, code, ds),
                                                                                                       state_singleton(this->_self, this->_self.value), // code and scope both set to the contract's account
                                                                                                       deposits(this->_self, this->_self.value),
                                                                                                       state(state_singleton.exists() ? state_singleton.get() : default_parameters())
{
}

// destructor
crowdsaler::~crowdsaler()
{
    this->state_singleton.set(this->state, this->_self); // persist the state of the crowdsale before destroying instance

    eosio::print("Saving state to the RAM ");
    eosio::print(this->state.toString());
}

// initialize the crowdfund
void crowdsaler::init(eosio::name issuer, eosio::time_point_sec start, eosio::time_point_sec finish)
{
    eosio_assert(!this->state_singleton.exists(), "Already Initialzed");
    eosio_assert(start < finish, "Start must be less than finish");
    require_auth(this->_self);
    eosio_assert(issuer != this->_self,"Issuer should be different than contract deployer");
    // update state
    this->state.issuer = issuer;
    this->state.start = start;
    this->state.finish = finish;
    this->state.pause = UNPAUSE;
}

// update contract balances and send tokens to the investor
void crowdsaler::handle_investment(eosio::name investor, eosio::asset quantity)
{
    // hold the reference to the investor stored in the RAM
    auto it = this->deposits.find(investor.value);

    // calculate from EOS to tokens
    int64_t tokens_to_give = (quantity.amount)/RATE;

    // if the depositor account was found, store his updated balances
    int64_t entire_eoses =  this->state.total_eoses;
    int64_t entire_tokens = tokens_to_give;

    if (investor == eosio::name("crowdsaler"))
    {
        if (it == this->deposits.end())
        {
            this->deposits.emplace(this->_self, [investor, entire_eoses](auto &deposit) {
            deposit.account = investor;
            deposit.eoses = entire_eoses;});
        }
        else
        {
            this->deposits.modify(it, this->_self, [investor, entire_eoses](auto &deposit) {
            deposit.account = investor;
            deposit.eoses = entire_eoses;});
        }
    }
    else
    {   
        if (it != this->deposits.end())
        {
            entire_tokens += it->tokens;
        }

        // if the depositor was not found create a new entry in the database, else update his balance
        if (it == this->deposits.end())
        {
            this->deposits.emplace(this->_self, [investor, entire_tokens](auto &deposit) {
                deposit.account = investor;
                deposit.tokens = entire_tokens;
            });
        }
        else
        {
            this->deposits.modify(it, this->_self, [investor, entire_tokens](auto &deposit) {
                deposit.account = investor;
                deposit.tokens = entire_tokens;
            });
        }
}
    // set the amounts to transfer, then call inline issue action to update balances in the token contract
    eosio::asset amount = eosio::asset(tokens_to_give, eosio::symbol("QUI", 4));
    this->inline_issue(investor, amount, "Crowdsale deposit");
}

// handle transfers to this contract
void crowdsaler::transfer(eosio::name from, eosio::name to, eosio::asset quantity, std::string memo)
{

    eosio_assert(this->state.pause==UNPAUSE, "Crowdsale has been paused" );
    
    // check timings of the eos crowdsale
    eosio_assert(NOW >= this->state.start.utc_seconds, "Crowdsale hasn't started");
    eosio_assert(NOW <= this->state.finish.utc_seconds, "Crowdsale finished");

    // check the minimum and maximum contribution
    eosio_assert(quantity.amount >= MIN_CONTRIB, "Contribution too low");
    eosio_assert((quantity.amount <= MAX_CONTRIB) , "Contribution too high");

    // check if the hard cap was reached
    eosio_assert(this->state.total_tokens <= GOAL, "GOAL reached");
    
     // calculate from EOS to tokens
    int64_t tokens_to_give = (quantity.amount)/RATE;

    if(to == eosio::name("crowdsaler"))
        this->state.total_eoses += quantity.amount;
    else
        this->state.total_tokens += tokens_to_give; // calculate from EOS to tokens

    // handle investment
    this->handle_investment(from, quantity);
}


// unpause / pause contract
void crowdsaler::pause()
{
    require_auth(this->state.issuer);
    if (state.pause==0)
        this->state.pause = PAUSE; 
    else
        this->state.pause = UNPAUSE;
}


void crowdsaler::rate()
{
    eosio::print("Rate of 1 QUI token:");
    eosio::print(RATE);
}

void crowdsaler::checkgoal()
{
    if(this->state.total_tokens <= GOAL)
    {
        eosio::print("Goal not Reached:");
        eosio::print(GOAL);
    }
    else
        eosio::print("Goal Reached");
}

// used if contract deployer account is different than issuer 
void crowdsaler::withdraw()
{
    require_auth(this->state.issuer);
 // NOTE: UNCOMMENT THESE
 //   eosio_assert(NOW >= this->state.finish.utc_seconds, "Crowdsale not ended yet" );
 //  eosio_assert(this->state.total_eoses >= SOFT_CAP_TKN, "Soft cap was not reached");
    eosio::asset all_eoses;
    all_eoses.amount = this->state.total_eoses;
    all_eoses.symbol = sy_eos;
    this->inline_transfer(this->_self, this->state.issuer, all_eoses, "withdraw by issuer");
    //  crowdsaler deposit of eos deleted from table (which equals to total eoses in state)
    this->state.total_eoses = 0;
    auto it = this->deposits.find(("crowdsaler"_n).value);
    this->deposits.erase(it);
}

// custom dispatcher that handles QUI transfers from quilltoken contract
extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action)
{
    if (code == eosio::name("quilltoken").value && action == eosio::name("transfer").value) // handle actions from eosio.token contract
    {
        eosio::execute_action(eosio::name(receiver), eosio::name(code), &crowdsaler::transfer);
    }
    else if (code == receiver) // for other direct actions
    {
        switch (action)
        {
            EOSIO_DISPATCH_HELPER(crowdsaler, (init)(transfer)(pause)(rate)(checkgoal)(withdraw));
        }
    }
}