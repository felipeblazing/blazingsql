#include "messageSender.hpp"

namespace comm {

message_sender * message_sender::instance = nullptr;


message_sender * message_sender::get_instance() {
	if(instance == NULL) {
        throw std::exception();
	}
	return instance;
}

message_sender *message_sender::get_instance(
		std::shared_ptr<ral::cache::CacheMachine> output_cacheC,
		std::map<std::string, node> node_address_map,
		int num_threads,
		ucp_context_h context,
		ucp_worker_h origin_node,
		int ral_id,
		comm::blazing_protocol protocol) {
	if (instance == nullptr) {
		std::cout << "get_instance: initializing sender" << std::endl;
		comm::message_sender::initialize_instance(output_cacheC,
		                                          node_address_map,
		                                          num_threads,
		                                          context,
		                                          origin_node,
		                                          ral_id,
		                                          protocol);
		std::cout << "get_instance: starting polling sender" << std::endl;
		instance->run_polling();
	}

	return instance;
}

void message_sender::initialize_instance(std::shared_ptr<ral::cache::CacheMachine> output_cache,
		std::map<std::string, node> node_address_map,
		int num_threads,
		ucp_context_h context,
		ucp_worker_h origin_node,
		int ral_id,
		comm::blazing_protocol protocol){
			std::cout<<"calling message_sender constructor"<<std::endl;
        message_sender::instance = new message_sender(
            output_cache,node_address_map,num_threads,context,origin_node,ral_id,protocol);
}

message_sender::message_sender(std::shared_ptr<ral::cache::CacheMachine> output_cache,
		const std::map<std::string, node> & node_address_map,
		int num_threads,
		ucp_context_h context,
		ucp_worker_h origin,
		int ral_id,
    comm::blazing_protocol protocol)
		: ral_id{ral_id}, origin{origin}, output_cache{output_cache}, node_address_map{node_address_map}, pool{num_threads}, protocol{protocol}, polling_thread_keep_running{true}, polling_thread_has_at_least_a_thread_pooled{false}
{
	std::cout<<"getting context query1"<<std::endl;
	request_size = 0;
	if (protocol == blazing_protocol::ucx)	{
		ucp_context_attr_t attr;
		std::cout<<"getting context query2"<<std::endl;
		attr.field_mask = UCP_ATTR_FIELD_REQUEST_SIZE;
		ucs_status_t status = ucp_context_query(context, &attr);
		if (status != UCS_OK)	{
			throw std::runtime_error("Error calling ucp_context_query");
		}

		request_size = attr.request_size;

		std::cout << "message_sender request_size: " << request_size << std::endl;
	}else{
		std::cout<<"Wroong protocol"<<std::endl;
	}
}

void message_sender::stop_polling() {
	std::cout << "Sender: Stopping polling" << std::endl;
	std::unique_lock<std::mutex> lock(polling_started_mutex);
	polling_started_condition.wait(lock);
	pool.stop(true);
	polling_thread_keep_running = false;
	output_cache->finish();
	std::cout << "Sender: Stopped polling" << std::endl;
}

} // namespace comm
