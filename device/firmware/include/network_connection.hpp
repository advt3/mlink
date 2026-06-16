#pragma once

class NetworkConnection {
public:
    NetworkConnection();
    bool initialize();
    bool is_connected() const;

private:
    bool m_initialized;
};
