#ifndef KERNEL_H
#define KERNEL_H

#include <string>
#include <vector>
#include <boost/compute/core.hpp>
#include <iostream>

/**
 * @brief A computing kernel.
 * 
 * Right now, only OpenCL kernels are supported. 
 */
class Kernel {
public:
    /**
     * @brief Create a new kernel.
     * @param device OpenCL device to use. If not given uses the system default.
     */
    Kernel(boost::compute::device device = boost::compute::system::default_device());

    /**
     * @brief Compile an OpenCL Kernel.
     * @param kernel Kernel source.
     */
    bool compile(const std::string &kernel);

    /**
     * @brief Add input parameters to the kernel.
     * @tparam T Kernel data type.
     * @param data An array of data to pass to the kernel.
     */
    template<typename T>
    void addInputData(std::vector<T> data) {
        const size_t size = data.size();
        const size_t typeSize = sizeof(T);
        const size_t totalSize = size * typeSize;
        void* ptr = nullptr;
    
        ptr = new T*[totalSize];
        memcpy(ptr, data.data(), totalSize);

        // create a memory buffer.
        boost::compute::buffer buffer(m_context, size * typeSize);
        const BufferInfo info = { buffer, ptr, size, typeSize };
        m_input.push_back(info);

        if (data.size() > m_work_size) {
            m_work_size = data.size();
        }
    }

    template<typename T>
    void addInputData(const uint64_t index, std::vector<T> data) {
        const size_t size = data.size();
        const size_t typeSize = sizeof(T);
        const size_t totalSize = size * typeSize;
        void* ptr = nullptr;
    
        ptr = new T*[totalSize];
        //memcpy(ptr, &data[0], totalSize);
        memcpy(ptr, data.data(), totalSize);

        // create a memory buffer.
        boost::compute::buffer buffer(m_context, size * typeSize);
        const BufferInfo info = { buffer, ptr, size, typeSize };
        m_input[index] = info;

        if (data.size() > m_work_size) {
            m_work_size = data.size();
        }
    }

    /**
     * @brief Add output parameters to a kernel.
     * @param params List of output sizes.
     */
    void addOutputParams(std::vector<size_t> params);

    /**
     * @brief Execute the compiled kernel.
     */
    void execute();

    /**
     * @brief Return output data from the kernel.
     * @param index Which output parameter to use.
     * @tparam T Data type to return.
     * @returns an array of T. Note that the user is expected to know the size
     * of the output, and to free the data when no longer in use.
     */
    template<typename T>
    T* getOutputData(size_t index) {
        const auto buffer = m_output[index];
        const auto bufferSize = buffer.size();

        size_t offset = 0;
        // TODO: use loop to discover offset. for(buffers-1) offset += buffer.size()

        T* result = new T[bufferSize];
        m_queue.enqueue_read_buffer(buffer, 0, bufferSize, result);

        return result;
    }

    void setInputSize(const size_t size) {
        m_input.resize(size);
    }

private:
    struct BufferInfo {
        boost::compute::buffer buffer;
        void* ptr;
        size_t size;
        size_t typeSize;
    };

    size_t m_work_size;
    boost::compute::device m_device;
    boost::compute::context m_context;
    boost::compute::command_queue m_queue;
    boost::compute::program m_program;
    boost::compute::kernel m_kernel;
    std::vector<BufferInfo> m_input;
    std::vector<size_t> m_outputSizes;
    std::vector<boost::compute::buffer> m_output;
};

#endif
