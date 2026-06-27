#pragma once

#include <zephyr/kernel.h>

class MenderClient {
public:
    MenderClient() = default;
    ~MenderClient() = default;

    // Disallow copy and assignment
    MenderClient(const MenderClient&) = delete;
    MenderClient& operator=(const MenderClient&) = delete;

    /**
     * @brief Initialize the Mender client client, registers callbacks.
     * @return true if initialization was successful, false otherwise.
     */
    bool initialize();

    /**
     * @brief Starts the Mender client worker thread.
     */
    void start();

private:
    static void thread_entry(void* p1, void* p2, void* p3);

    k_tid_t m_thread_id{nullptr};
    struct k_thread m_thread_data{};
    K_KERNEL_STACK_MEMBER(m_stack, 8192);
};

