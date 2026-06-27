#include "mender_client.hpp"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/net/tls_credentials.h>

LOG_MODULE_REGISTER(mender_client, LOG_LEVEL_INF);

extern "C" {
#include <mender/client.h>
#include <mender/zephyr-image-update-module.h>
}

// ISRG Root X1 (Let's Encrypt Root CA) for eu.hosted.mender.io
static const char mender_ca_cert_primary[] =
    "-----BEGIN CERTIFICATE-----\n"
    "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\n"
    "-----END CERTIFICATE-----\n";

// Amazon Root CA 1 for artifact S3 download domain
static const char mender_ca_cert_secondary[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
    "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
    "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
    "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
    "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
    "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
    "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
    "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
    "-----END CERTIFICATE-----\n";

static int register_mender_credentials() {
    int rc = tls_credential_add(CONFIG_MENDER_NET_CA_CERTIFICATE_TAG_PRIMARY, TLS_CREDENTIAL_CA_CERTIFICATE,
                                mender_ca_cert_primary, sizeof(mender_ca_cert_primary));
    if (rc < 0 && rc != -EEXIST) {
        LOG_ERR("Failed to register Mender primary CA certificate: %d", rc);
        return rc;
    }

    rc = tls_credential_add(CONFIG_MENDER_NET_CA_CERTIFICATE_TAG_SECONDARY, TLS_CREDENTIAL_CA_CERTIFICATE,
                            mender_ca_cert_secondary, sizeof(mender_ca_cert_secondary));
    if (rc < 0 && rc != -EEXIST) {
        LOG_ERR("Failed to register Mender secondary CA certificate: %d", rc);
        return rc;
    }

    return 0;
}

#include <zephyr/net/net_if.h>
#include <stdio.h>

static char g_mac_address[18] = "00:00:00:00:00:00";

// C-compatible callbacks for the Mender MCU Client
static mender_err_t get_identity_cb(const mender_identity_t** identity) {
    static mender_identity_t device_identity = {
        .name = const_cast<char*>("mac"),
        .value = g_mac_address
    };
    if (identity != nullptr) {
        *identity = &device_identity;
        return MENDER_OK;
    }
    return MENDER_FAIL;
}

static mender_err_t reboot_cb() {
    LOG_INF("Mender client requested reboot. Restarting system...");
    sys_reboot(SYS_REBOOT_COLD);
    return MENDER_OK;
}

bool MenderClient::initialize() {
    struct net_if *iface = net_if_get_default();
    if (iface) {
        struct net_linkaddr *link_addr = net_if_get_link_addr(iface);
        if (link_addr && link_addr->len == 6) {
            snprintf(g_mac_address, sizeof(g_mac_address), "%02X:%02X:%02X:%02X:%02X:%02X",
                     link_addr->addr[0], link_addr->addr[1], link_addr->addr[2],
                     link_addr->addr[3], link_addr->addr[4], link_addr->addr[5]);
        }
    }

    if (register_mender_credentials() < 0) {
        LOG_ERR("Failed to register Mender TLS credentials");
        return false;
    }

    mender_client_config_t mender_client_config = {
        .device_type = const_cast<char*>("esp32c6_devkitc")
    };


    mender_client_callbacks_t mender_callbacks = {
        .network_connect = nullptr,
        .network_release = nullptr,
        .deployment_status = nullptr,
        .restart = reboot_cb,
        .get_identity = get_identity_cb,
        .get_user_provided_keys = nullptr
    };

    if (mender_client_init(&mender_client_config, &mender_callbacks) != MENDER_OK) {
        LOG_ERR("Failed to initialize Mender client");
        return false;
    }

    if (mender_zephyr_image_register_update_module() != MENDER_OK) {
        LOG_ERR("Failed to register Mender Zephyr image update module");
        return false;
    }

    LOG_INF("Mender client successfully initialized");
    return true;
}

void MenderClient::thread_entry(void* p1, void* p2, void* p3) {
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    LOG_INF("Mender client thread running, waiting for network interface...");
    
    // Initial delay to let the network connection settle
    k_sleep(K_SECONDS(15));

    LOG_INF("Activating Mender client...");
    mender_client_activate();
}

void MenderClient::start() {
    m_thread_id = k_thread_create(
        &m_thread_data,
        m_stack,
        K_THREAD_STACK_SIZEOF(m_stack),
        thread_entry,
        nullptr, nullptr, nullptr,
        K_PRIO_PREEMPT(5),
        0,
        K_NO_WAIT
    );
}
