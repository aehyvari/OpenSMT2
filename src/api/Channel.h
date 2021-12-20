#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>
#include "atomic"
#include "deque"
#include <map>

class Channel {
    using td_t = const std::chrono::duration<double>;
    mutable std::mutex              mutex;
    mutable std::condition_variable cv;

    std::deque<std::string>         queries;
    std::atomic_bool                requestStop;
    std::atomic_bool                stop;
    bool                            isClauseShareMode;
    bool                            injectClause;
    int                             clauseLearnInterval;
    std::string                     workingNode;
    std::map<std::string, std::vector<std::pair<std::string ,int>>> clauses;

public:
    void assign(std::vector<std::pair<std::string ,int>> toPublishTerms)
    {
        clauses[workingNode].insert(std::end(clauses[workingNode]), std::begin(toPublishTerms), std::end(toPublishTerms));
    }

    Channel() : requestStop(false), stop(false), isClauseShareMode(false), injectClause(false),
                clauseLearnInterval(1000) {
    }

    std::map<std::string, std::vector<std::pair<std::string ,int>>> & getClauseMap() { return clauses; };
    std::mutex & getMutex()                 { return mutex; }
    size_t size() const                     { return clauses.size(); }
    auto cbegin() const                     { return clauses.cbegin(); }
    auto cend() const                       { return clauses.cend(); }
    auto begin() const                      { return clauses.begin(); }
    auto end() const                        { return clauses.end(); }
    void notify_one()                       { cv.notify_one(); }
    void notify_all()                       { cv.notify_all(); }
    void clear()                            { clauses.clear(); }
    bool empty() const                      { return clauses.empty(); }
    bool shouldTerminate() const            { return stop; }
    void setTerminate()                     { stop = true; }
    void clearTerminate()                   { stop = false; }
    bool shouldStop() const                 { return requestStop; }
    void setShouldStop()                    { requestStop = true; }
    void clearShouldStop()                  { requestStop = false; }
    bool shallClauseShareMode() const       { return isClauseShareMode; }
    void setClauseShareMode()               { isClauseShareMode = true; }
    void clearClauseShareMode()             { isClauseShareMode = false; }
    void pop_front_query()                  { queries.pop_front();}
    size_t size_query() const               { return queries.size(); }
    void setWorkingNode(std::string& n)     { workingNode = n; }
    std::string getWorkingNode() const      { return workingNode; }
    bool isEmpty_query() const              { return queries.empty(); }
    void clear_query()                      { queries.clear() ; }
    auto cbegin_query() const               { return queries.cbegin(); }
    auto cend_query() const                 { return queries.cend(); }
    auto get_queris() const                 { return queries; }
    bool shouldInjectClause() const                 { return injectClause; }
    void setInjectClause()                          { injectClause = true; }
    void clearInjectClause()                        { injectClause = false; }
    void setClauseLearnInterval(int wait)           { clauseLearnInterval = wait; }
    int getClauseLearnInterval() const              { return clauseLearnInterval; }
    void push_back_query(const std::string& str)    { queries.push_back(str);}
    void resetChannel()
    {
        clear();
        clear_query();
        clearShouldStop();
        clearTerminate();
        clearClauseShareMode();
        clearInjectClause();
    }

    bool waitFor(std::unique_lock<std::mutex> & lock, const td_t& timeout_duration)
    {
        return cv.wait_for(lock, timeout_duration, [&] { return shouldTerminate(); });
    }
    void waitForQueryOrTemination(std::unique_lock<std::mutex> & lock)
    {
        cv.wait(lock, [&] { return (shouldTerminate() or not isEmpty_query());});
    }
};
