#include <string>
#include <algorithm>

#include "Log.h"
#include "View.h"
#include "Mixer.h"
#include "tinyxml2Toolkit.h"
#include "SessionVisitor.h"
#include "SessionCreator.h"

#include "ActionManager.h"

#ifndef NDEBUG
#define ACTION_DEBUG
#endif

using namespace tinyxml2;

Action::Action(): step_(0), max_step_(0)
{
}

void Action::clear()
{
    // clean the history
    xmlDoc_.Clear();
    step_ = 0;
    max_step_ = 0;

    // start fresh
    store("Session start");
}

void Action::store(const std::string &label, int id)
{
    // ignore if locked or if no label is given
    if (locked_ || label.empty())
        return;

    // incremental naming of history nodes
    step_++;
    std::string nodename = "H" + std::to_string(step_);

    // erase future
    for (uint e = step_; e <= max_step_; e++) {
        std::string name = "H" + std::to_string(e);
        XMLElement *node = xmlDoc_.FirstChildElement( name.c_str() );
        if ( node )
            xmlDoc_.DeleteChild(node);
    }
    max_step_ = step_;

    // create history node
    XMLElement *sessionNode = xmlDoc_.NewElement( nodename.c_str() );
    xmlDoc_.InsertEndChild(sessionNode);
    // label describes the action
    sessionNode->SetAttribute("label", label.c_str());
    // id indicates which object was modified
    sessionNode->SetAttribute("id", id);

    // get session to operate on
    Session *se = Mixer::manager().session();

    // save all sources using source visitor
    SessionVisitor sv(&xmlDoc_, sessionNode);
    for (auto iter = se->begin(); iter != se->end(); iter++, sv.setRoot(sessionNode) )
        (*iter)->accept(sv);

    // debug
#ifdef ACTION_DEBUG
    Log::Info("Action stored %s '%s'", nodename.c_str(), label.c_str());
    //    XMLSaveDoc(&xmlDoc_, "/home/bhbn/history.xml");
#endif
}

void Action::undo()
{
    if (step_ <= 1)
        return;

    // what id was modified to get to this step ?
    // get history node of current step
    std::string nodename = "H" + std::to_string(step_);
    XMLElement *sessionNode = xmlDoc_.FirstChildElement( nodename.c_str() );
    int id = -1;
    sessionNode->QueryIntAttribute("id", &id);

    restore( step_ - 1, id);
}

void Action::redo()
{
    if (step_ >= max_step_)
        return;

    // what id to modify to go to next step ?
    std::string nodename = "H" + std::to_string(step_ + 1);
    XMLElement *sessionNode = xmlDoc_.FirstChildElement( nodename.c_str() );
    int id = -1;
    sessionNode->QueryIntAttribute("id", &id);

    restore( step_ + 1, id);
}


void Action::stepTo(uint target)
{
    // going to target step
    restore(target, -1);
}


std::string Action::label(uint s) const
{
    std::string l = "";

    if (s > 0 && s <= max_step_) {
        std::string nodename = "H" + std::to_string(s);
        const XMLElement *sessionNode = xmlDoc_.FirstChildElement( nodename.c_str() );
        l = sessionNode->Attribute("label");
    }
    return l;
}

void Action::restore(uint target, int id)
{
    // lock
    locked_ = true;

    // get history node of target step
    step_ = CLAMP(target, 1, max_step_);
    std::string nodename = "H" + std::to_string(step_);
    XMLElement *sessionNode = xmlDoc_.FirstChildElement( nodename.c_str() );

#ifdef ACTION_DEBUG
    Log::Info("Restore %s '%s'", nodename.c_str(), sessionNode->Attribute("label"));
#endif

    // sessionsources contains ids of all sources currently in the session
    std::list<int> sessionsources = Mixer::manager().session()->getIdList();

    // load history status:
    // - if a source exists, its attributes are updated
    // - if a source does not exists (in session), it is created in session
    SessionLoader loader( Mixer::manager().session() );
    loader.load( sessionNode );

    // loadersources contains ids of all sources generated by loader
    std::list<int> loadersources = loader.getIdList();

    // remove intersect of both lists (sources were updated)
    for( auto lsit = loadersources.begin(); lsit != loadersources.end(); ){
        auto ssit = std::find(sessionsources.begin(), sessionsources.end(), (*lsit));
        if ( ssit != sessionsources.end() ) {
            lsit = loadersources.erase(lsit);
            sessionsources.erase(ssit);
        }
        else
            lsit++;
    }
    // remaining ids in list sessionsources : to remove
    while ( !sessionsources.empty() ){
        Source *s = Mixer::manager().findSource( sessionsources.front() );
        Mixer::manager().detach( s );
        Mixer::manager().session()->deleteSource( s );
        sessionsources.pop_front();
    }
    // remaining ids in list loadersources : to add
    while ( !loadersources.empty() ){
        Mixer::manager().attach( Mixer::manager().findSource( loadersources.front() ) );
        loadersources.pop_front();
    }

    // free
    locked_ = false;

}
