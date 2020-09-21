#pragma once

#include <memory>
#include <map>

#include "utilities/ctpl_stl.h"
#include "blazingdb/concurrency/BlazingThread.h"
#include <ucp/api/ucp.h>
#include <ucp/api/ucp_def.h>
#include "messageReceiver.hpp"

namespace comm {

class message_listener{
public:
    message_listener(const std::map<std::string, comm::node>& nodes, int num_threads) : _nodes_info_map{nodes}, pool{num_threads} {}
    virtual ~message_listener(){

    }
    virtual void start_polling() = 0;
    virtual void stop_polling() = 0;
    ctpl::thread_pool<BlazingThread> & get_pool();
    std::map<std::string, comm::node> get_node_map();

protected:
    ctpl::thread_pool<BlazingThread> pool;
    std::map<std::string, comm::node> _nodes_info_map;
};

class tcp_message_listener : public message_listener {

public:
    static void initialize_message_listener(const std::map<std::string, comm::node>& nodes, int port, int num_threads);
    static tcp_message_listener * get_instance();
    void start_polling() override;
    void stop_polling() override;
    virtual ~tcp_message_listener(){

    }
private:
    tcp_message_listener(const std::map<std::string, comm::node>& nodes, int port, int num_threads);
    int _port;
    static tcp_message_listener * instance;
};



class ucx_message_listener : public message_listener {
public:

    static void initialize_message_listener(ucp_context_h context, ucp_worker_h worker, const std::map<std::string, comm::node>& nodes, int num_threads);
    static ucx_message_listener * get_instance();
	  static ucx_message_listener *
	  get_instance(ucp_context_h context,
	               ucp_worker_h worker,
	               const std::map<std::string, comm::node> &nodes_info_map,
	               int num_threads);

	  void poll_begin_message_tag(bool running_from_unit_test);
    void add_receiver(ucp_tag_t tag,std::shared_ptr<message_receiver> receiver);
    void remove_receiver(ucp_tag_t tag);
    void increment_frame_receiver(ucp_tag_t tag);
    ucp_worker_h get_worker();
    void start_polling() override;
    void stop_polling() override;
private:
    ucx_message_listener(ucp_context_h context, ucp_worker_h worker, const std::map<std::string, comm::node>& nodes, int num_threads);
	virtual ~ucx_message_listener(){

    }
    void poll_message_tag(ucp_tag_t tag, ucp_tag_t mask);
    size_t _request_size;
    ucp_worker_h ucp_worker;
    std::map<ucp_tag_t,std::shared_ptr<message_receiver> > tag_to_receiver;
	static ucx_message_listener * instance;
    std::thread poll_begin_thread;
    bool poll_begin_thread_keep_running;
};

} // namespace comm
