#include "kernel.h"
#include <utility>
#include <iostream>
#include <iterator>

namespace compute = boost::compute;

Kernel::Kernel(boost::compute::device device)
    : m_device(device)
    , m_context(m_device)
    , m_queue(m_context, m_device)
    , m_program()
    , m_kernel()
    , m_work_size(0)
{
}

bool Kernel::compile(const std::string &kernel) {
    try {
        m_program = compute::program::create_with_source(kernel, m_context);
        m_program.build();
        m_kernel = boost::compute::kernel(m_program, "add");
        return true;
    } catch (...) {
        return false;
    }
}

// TODO: append, not replace.
void Kernel::addOutputParams(std::vector<size_t> params) {
    m_outputSizes = params;
}

void Kernel::execute() {
    // setup input arrays

    std::cout << "pre-input\n";
    std::cout << "Size: " << m_input.size() << "\n";
    for(auto i=0; i<m_input.size(); i++)
    {
        std::cout << "  0\n";
        auto d = m_input[i];
        std::cout << "  1\n";

        // Transfer the data the host to the device.
        m_queue.enqueue_write_buffer(d.buffer, 0, d.size * d.typeSize, d.ptr);
        std::cout << "  2\n";
        // Set the kernel argument.
        m_kernel.set_arg(i, d.buffer);
    }

    std::cout << "pre-output\n";
    // Make space for the output
    const size_t outputStart = m_input.size();
    const size_t outputEnd = outputStart + 1;
    for (auto i=outputStart; i<outputEnd; i++)
    {
        const auto sizeOffset = i - outputStart;
        const auto size = m_outputSizes[sizeOffset];

        compute::buffer buffer(m_context, size);
        m_kernel.set_arg(i, buffer);
        m_output.push_back(buffer);
    }

    // run the add kernel
    std::cout << "pre-enqueue\n";
    m_queue.enqueue_1d_range_kernel(m_kernel, 0, m_work_size, 0);

    for(size_t i=0; i<m_input.size(); i++) {
        // delete ptr
    }
}
