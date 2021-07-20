/*
https://github.com/nghttp2/nghttp2/tree/v1.43.0
https://nghttp2.org/
https://github.com/nghttp2/nghttp2/blob/master/examples/asio-sv.cc

https://github.com/Corvusoft/restbed/blob/master/documentation/example/LOGGING.md
https://github.com/Corvusoft/restbed/blob/master/documentation/example/MULTITHREADED_SERVICE.md
https://github.com/Corvusoft/restbed/blob/master/documentation/example/RULES_ENGINE.md
https://github.com/Corvusoft/restbed/blob/master/documentation/example/SCHEDULE_WORK.md

*/
#include <map>
#include <mutex>
#include <chrono>
#include <string>
#include <memory>
#include <random>
#include <cstdlib>

using namespace std;

//using Callback = const function< void ( const shared_ptr< float > ) >&;

#if 1
int main() {
    return 0;
}
#else
#include <restbed>

using namespace restbed;

void post_method_handler(const shared_ptr<restbed::Session> session) {
    const auto request = session->get_request();

    size_t content_length = request->get_header("Content-Length", 0);

    session->fetch(content_length, [request](const shared_ptr<Session> session, const Bytes & body)
    {
        fprintf(stdout, "%.*s\n", (int) body.size(), body.data());
        session->close(OK, "Hello, World!", { { "Content-Length", "13" }, { "Connection", "close" } });
    });
}

class InMemorySessionManager : public SessionManager
{
    public:
        InMemorySessionManager( void ) : m_sessions_lock( ),
            m_sessions( )
        {
            return;
        }
        
        ~InMemorySessionManager( void )
        {
            return;
        }
        
        void start( const shared_ptr< const Settings >& )
        {
            return;
        }
        
        void create( const function< void ( const shared_ptr< Session > ) >& callback )
        {
            static const string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            static uniform_int_distribution< > selector( 0, 35 );
            
            auto seed = static_cast< unsigned int >( chrono::high_resolution_clock::now( ).time_since_epoch( ).count( ) );
            static mt19937 generator( seed );
            
            string key = "";
            
            for ( int index = 0; index < 32; index++ )
            {
                key += charset.at( selector( generator ) );
            }
            
            callback( make_shared< Session >( key ) );
        }
        
        void load( const shared_ptr< Session > session, const function< void ( const shared_ptr< Session > ) >& callback )
        {
            const auto request = session->get_request( );
            
            unique_lock< mutex > lock( m_sessions_lock );
            auto previous_session = m_sessions.find( request->get_header( "SessionID" ) );
            
            if ( previous_session not_eq m_sessions.end( ) )
            {
                const auto id = previous_session->second->get_id( );
                session->set_id( id );
                
                for ( const auto key : previous_session->second->keys( ) )
                {
                    session->set( key, previous_session->second->get( key ) );
                }
            }
            
            lock.unlock( );
            
            const auto key = session->get_id( );
            session->set_header( "SessionID", key );
            
            callback( session );
        }
        
        void save( const shared_ptr< Session > session, const function< void ( const shared_ptr< Session > ) >& callback )
        {
            unique_lock< mutex > lock( m_sessions_lock );
            m_sessions[ session->get_id( ) ] = session;
            lock.unlock( );
            
            callback( session );
        }
        
        void stop( void )
        {
            return;
        }
        
    private:
        mutex m_sessions_lock;
        
        map< string, shared_ptr< Session > > m_sessions;
};

void session_handler(const shared_ptr<Session> session)
{
    string body = "Previous Session Data\n";
    
    for (const auto key : session->keys())
    {
        string value = session->get(key);
        body += key + "=" + value + "\n";
    }
    
    const auto request = session->get_request();
    
    for (const auto query_parameter : request->get_query_parameters())
    {
        session->set(query_parameter.first, query_parameter.second);
    }
    
    session->close(OK, body, { { "Connection", "close" } });
}

void index_handler(const shared_ptr<Session> session)
{
    session->close(OK, "Hello, World!", { { "Content-Length", "13" } });
}

int main(const int argc, const char** argv) {
    auto index = make_shared<Resource>();
    index->set_path("/");
    index->set_method_handler("GET", index_handler);

    auto post = make_shared<Resource>();
    post->set_path("/post");
    post->set_method_handler("POST", post_method_handler);

    auto session = make_shared<Resource>();
    session->set_path("/session");
    session->set_method_handler("GET", session_handler);

    auto settings = make_shared<Settings>();
    settings->set_port(1984);

    Service service;
    service.publish(post);
    service.publish(session);
    service.set_session_manager(make_shared<InMemorySessionManager>());
    service.start();

    return EXIT_SUCCESS;
}
#endif
