#pramga once

#include "nvcomp/cascaded.hpp"
#include "nvcomp/nvcomp.hpp"
#include <limits>


enum compression_type {
    LZ4,
    CASCADED
};

struct compression_plan{
    int num_rle;
    int num_delta;
    compression_type type;
};

struct{

}

static std::shared_ptr<PinnedBufferProvider> output_size_buffer{};

//size of 8 buffers for outputsize
PinnedBufferProvider &get_output_size_buffer_provider() { 
    if (output_size_buffer == nullptr){
        output_size_buffer = std::make_shared<PinnedBufferProvider>(sizeof(size_t),1000);
    }
    return *output_size_buffer; 
}



struct get_optimal_compression_functor{
    template <typename T>
    std::pair<compression_plan, rmm::device_buffer> operator()(
        T * data, 
        size_t num_elements, 
        cudaStream_t stream){ 

        //Try cascaded
        int max_rles = 3;
        int max_deltas = 3;
        int optimal_rle;
        int optimal_deltas;
        rmm::device_buffer<char> temp_space;
        rmm::device_buffer<char> temp_out;
        rmm::device_buffer<char> output;
        size_t min_output_size = std::numeric_limits<size_t>::max();

        for(int rle_count = max_rles; rle_count >= 0; rle_count--){
            for(int delta_count = max_deltas; delta_count >= max_detlas; delta_count < max_deltas){
                nvcomp::CascadedCompressor<T> compressor(
                    data, num_elements, rle_count, delta_count, true);
                    
                    auto temp_size = compressor.get_temp_size();
                    if(temp_size > temp_space.size()){
                        temp_space.resize(temp_size);
                    }
                    
                    auto output_size_buffer = get_output_size_buffer_provider().getBuffer();
                    
                    size_t output_size = compressor.get_max_output_size(
                        temp_space.data(), temp_size);
                    if(temp_out.size() < output_size){
                        temp_out.resize(output_size);
                    }

                    compressor.compress_async(temp_space.data(),
                    temp_size, temp_out.data(), output_size_buffer.data(), stream);

                    output_size = *((size_t *) output_size_buffer.data());
                    if(output_size < min_output_size){
                        optimal_rle = rle_count;
                        optimal_deltas = delta_count;

                        min_output_size = output_size;
                        cudaStreamSynchronize(stream); //to make sure the previous copy is done before we change the allocation
                        output.resize(output_size);
                        cudaMemcpyAsync(output.data(), output_space.data(), output_size, cudaMemcpyDeviceToDevice,stream);
                    }
            }
        } 
        
        if((sizeof(T) * num_elements ) < output.size() ){
            //compression did not occur try lz4
            nvcomp::LZ4Compressor<T> compressor(data, num_elements,4096);

            auto temp_size = compressor.get_temp_size();
            if(temp_size > temp_space.size()){
                temp_space.resize(temp_size);
            }
            
            auto output_size_buffer = get_output_size_buffer_provider().getBuffer();
            
            size_t output_size = compressor.get_max_output_size(
                temp_space.data(), temp_size);
            if(temp_out.size() < output_size){
                temp_out.resize(output_size);
            }

            compressor.compress_async(temp_space.data(),
            temp_size, temp_out.data(), output_size_buffer.data(), stream);

            output_size = *((size_t *) output_size_buffer.data());
            min_output_size = output_size;
            cudaStreamSynchronize(stream); //to make sure the previous copy is done before we change the allocation
            output.resize(output_size);
            cudaMemcpyAsync(output.data(), output_space.data(), output_size, cudaMemcpyDeviceToDevice,stream);

            cudaStreamSynchronize(stream); 
            return std::make_pair<com>pression_plan, rmm::device_buffer>({0,0,compression_type::LZ4},std::move(output));            
        }else{
            cudaStreamSynchronize(stream); 
            return std::make_pair<com>pression_plan, rmm::device_buffer>({optimal_rle,optimal_deltas,compression_type::CASCADED},std::move(output));
        }
               
    }
};

template <typename T>
  get_optimal_compression_plan(){

}