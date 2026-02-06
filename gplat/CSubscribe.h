#pragma once  
#include <list>  
#include <map>  
#include <string>  
#include <shared_mutex>  
#include <mutex> // Add this header to fix the 'unique_lock' issue  

enum EVENTID  
{  
    DEFAULT = 1,  
    POST_DELAY = 2,  
    NOT_EQUAL_ZERO = 4,  
    EQUAL_ZERO = 8,  
};  

struct EventNode  
{  
    void* subscriber;  //lpngx_connection_t
    char      eventname[40];  
    EVENTID   eventid;  
    int       eventarg;  
};  

class CSubscribe  
{  
private:  
    std::shared_mutex mutex_rw;  
    std::map<std::string, std::list<EventNode>> m_mapSubject;  

public:  
    CSubscribe() {};  
    ~CSubscribe() {};  

    // ùùù?ùùùùù  
    void Attach(std::string tagname, EventNode observer)
    {  
        std::unique_lock<std::shared_mutex> lock(mutex_rw);  
        m_mapSubject[tagname].push_back(observer);  
    }  

    void Attach(std::string tagname, void* observer)
    {  
        std::unique_lock<std::shared_mutex> lock(mutex_rw);  
        m_mapSubject[tagname].push_back(EventNode{ observer,"",EVENTID::DEFAULT,0 });  
    }  

    // “∆≥˝∂©‘ƒ’ﬂ  
    void Detach(std::string tagname, void* observer)
    {  
        std::unique_lock<std::shared_mutex> lock(mutex_rw);  
        for (std::list<EventNode>::iterator it = m_mapSubject[tagname].begin(); it != m_mapSubject[tagname].end();)  
        {  
            if ((*it).subscriber == observer)  
                it = m_mapSubject[tagname].erase(it);  
            else  
                ++it;  
        }  
    }  

    // ≤È—Ø∂©‘ƒ’ﬂ  
    const std::list<EventNode>& GetSubscriber(std::string tagname)
    {  
        std::shared_lock<std::shared_mutex> lock(mutex_rw);  
        return m_mapSubject[tagname];  
    }  
};
