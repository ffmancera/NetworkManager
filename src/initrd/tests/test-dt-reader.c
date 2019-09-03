/* NetworkManager initrd configuration generator
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * Copyright 2014 - 2018 Red Hat, Inc.
 */

#include "nm-default.h"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "nm-core-internal.h"
#include "NetworkManagerUtils.h"

#include "../nm-initrd-generator.h"

#include "nm-test-utils-core.h"

#if 0
static NMConnection *
read_connection (const char *sysfs_dir, const char *expected_mac, GError **error)
{
	NMConnection *connection = NULL;
	gs_unref_hashtable GHashTable *ibft = NULL;
	gs_free char *mac = NULL;
	GHashTable *nic = NULL;

	ibft = nmi_ibft_read (sysfs_dir);

	mac = g_ascii_strup (expected_mac, -1);
	nic = g_hash_table_lookup (ibft, mac);
	if (!nic)
		return NULL;

	connection = nm_simple_connection_new ();

	if (!nmi_ibft_update_connection_from_nic (connection, nic, error))
		g_clear_object (&connection);

	return connection;
}

static void
test_read_ibft_dhcp (void)
{
	NMConnection *connection;
	NMSettingConnection *s_con;
	NMSettingWired *s_wired;
	NMSettingIPConfig *s_ip4;
	NMSettingIPConfig *s_ip6;
	GError *error = NULL;
	const char *mac_address;
	const char *expected_mac_address = "00:33:21:98:b9:f1";

	connection = read_connection (TEST_INITRD_DIR "/sysfs-dhcp", expected_mac_address, &error);
	g_assert_no_error (error);
	nmtst_assert_connection_verifies_without_normalization (connection);

	g_assert (!nm_connection_get_setting_vlan (connection));

	s_con = nm_connection_get_setting_connection (connection);
	g_assert (s_con);
	g_assert_cmpstr (nm_setting_connection_get_connection_type (s_con), ==, NM_SETTING_WIRED_SETTING_NAME);
	g_assert_cmpstr (nm_setting_connection_get_id (s_con), ==, "iBFT Connection 1");
	g_assert_cmpint (nm_setting_connection_get_timestamp (s_con), ==, 0);
	g_assert (nm_setting_connection_get_autoconnect (s_con));

	s_wired = nm_connection_get_setting_wired (connection);
	g_assert (s_wired);
	mac_address = nm_setting_wired_get_mac_address (s_wired);
	g_assert (mac_address);
	g_assert (nm_utils_hwaddr_matches (mac_address, -1, expected_mac_address, -1));
	g_assert_cmpint (nm_setting_wired_get_mtu (s_wired), ==, 0);

	s_ip4 = nm_connection_get_setting_ip4_config (connection);
	g_assert (s_ip4);
	g_assert_cmpstr (nm_setting_ip_config_get_method (s_ip4), ==, NM_SETTING_IP4_CONFIG_METHOD_AUTO);

	s_ip6 = nm_connection_get_setting_ip6_config (connection);
	g_assert (s_ip6);
	g_assert_cmpstr (nm_setting_ip_config_get_method (s_ip6), ==, NM_SETTING_IP6_CONFIG_METHOD_DISABLED);

	g_object_unref (connection);
}

static void
test_read_ibft_static (void)
{
	NMConnection *connection;
	NMSettingConnection *s_con;
	NMSettingWired *s_wired;
	NMSettingIPConfig *s_ip4;
	NMSettingIPConfig *s_ip6;
	GError *error = NULL;
	const char *mac_address;
	const char *expected_mac_address = "00:33:21:98:b9:f0";
	NMIPAddress *ip4_addr;

	connection = read_connection (TEST_INITRD_DIR "/sysfs-static", expected_mac_address, &error);
	g_assert_no_error (error);
	nmtst_assert_connection_verifies_without_normalization (connection);

	g_assert (!nm_connection_get_setting_vlan (connection));

	s_con = nm_connection_get_setting_connection (connection);
	g_assert (s_con);
	g_assert_cmpstr (nm_setting_connection_get_connection_type (s_con), ==, NM_SETTING_WIRED_SETTING_NAME);
	g_assert_cmpstr (nm_setting_connection_get_id (s_con), ==, "iBFT Connection 0");
	g_assert_cmpint (nm_setting_connection_get_timestamp (s_con), ==, 0);
	g_assert (nm_setting_connection_get_autoconnect (s_con));

	s_wired = nm_connection_get_setting_wired (connection);
	g_assert (s_wired);
	mac_address = nm_setting_wired_get_mac_address (s_wired);
	g_assert (mac_address);
	g_assert (nm_utils_hwaddr_matches (mac_address, -1, expected_mac_address, -1));
	g_assert_cmpint (nm_setting_wired_get_mtu (s_wired), ==, 0);

	s_ip4 = nm_connection_get_setting_ip4_config (connection);
	g_assert (s_ip4);
	g_assert_cmpstr (nm_setting_ip_config_get_method (s_ip4), ==, NM_SETTING_IP4_CONFIG_METHOD_MANUAL);

	g_assert_cmpint (nm_setting_ip_config_get_num_dns (s_ip4), ==, 2);
	g_assert_cmpstr (nm_setting_ip_config_get_dns (s_ip4, 0), ==, "10.16.255.2");
	g_assert_cmpstr (nm_setting_ip_config_get_dns (s_ip4, 1), ==, "10.16.255.3");

	g_assert_cmpint (nm_setting_ip_config_get_num_addresses (s_ip4), ==, 1);
	ip4_addr = nm_setting_ip_config_get_address (s_ip4, 0);
	g_assert (ip4_addr);
	g_assert_cmpstr (nm_ip_address_get_address (ip4_addr), ==, "192.168.32.72");
	g_assert_cmpint (nm_ip_address_get_prefix (ip4_addr), ==, 22);

	g_assert_cmpstr (nm_setting_ip_config_get_gateway (s_ip4), ==, "192.168.35.254");

	s_ip6 = nm_connection_get_setting_ip6_config (connection);
	g_assert (s_ip6);
	g_assert_cmpstr (nm_setting_ip_config_get_method (s_ip6), ==, NM_SETTING_IP6_CONFIG_METHOD_DISABLED);

	g_object_unref (connection);
}


static void
test_read_ibft_bad_address (gconstpointer user_data)
{
	const char *sysfs_dir = user_data;
	NMConnection *connection;
	GError *error = NULL;

	g_assert (g_file_test (sysfs_dir, G_FILE_TEST_EXISTS));

	connection = read_connection (sysfs_dir, "00:33:21:98:b9:f0", &error);
	g_assert (connection == NULL);
	g_assert (error);
	g_clear_error (&error);
}

static void
test_read_ibft_vlan (void)
{
	NMConnection *connection;
	NMSettingConnection *s_con;
	NMSettingWired *s_wired;
	NMSettingVlan *s_vlan;
	NMSettingIPConfig *s_ip4;
	const char *mac_address;
	const char *expected_mac_address = "00:33:21:98:b9:f0";
	NMIPAddress *ip4_addr;
	GError *error = NULL;

	connection = read_connection (TEST_INITRD_DIR "/sysfs-vlan", expected_mac_address, &error);
	g_assert_no_error (error);
	nmtst_assert_connection_verifies_without_normalization (connection);

	s_con = nm_connection_get_setting_connection (connection);
	g_assert (s_con);
	g_assert_cmpstr (nm_setting_connection_get_connection_type (s_con), ==, NM_SETTING_VLAN_SETTING_NAME);

	/* ===== WIRED SETTING ===== */
	s_wired = nm_connection_get_setting_wired (connection);
	g_assert (s_wired);
	mac_address = nm_setting_wired_get_mac_address (s_wired);
	g_assert (mac_address);
	g_assert (nm_utils_hwaddr_matches (mac_address, -1, expected_mac_address, -1));

	/* ===== VLAN SETTING ===== */
	s_vlan = nm_connection_get_setting_vlan (connection);
	g_assert (s_vlan);
	g_assert_cmpint (nm_setting_vlan_get_id (s_vlan), ==, 123);
	g_assert_cmpstr (nm_setting_vlan_get_parent (s_vlan), ==, NULL);

	/* ===== IPv4 SETTING ===== */
	s_ip4 = nm_connection_get_setting_ip4_config (connection);
	g_assert (s_ip4);
	g_assert_cmpstr (nm_setting_ip_config_get_method (s_ip4), ==, NM_SETTING_IP4_CONFIG_METHOD_MANUAL);

	g_assert_cmpint (nm_setting_ip_config_get_num_dns (s_ip4), ==, 0);

	g_assert_cmpint (nm_setting_ip_config_get_num_addresses (s_ip4), ==, 1);
	ip4_addr = nm_setting_ip_config_get_address (s_ip4, 0);
	g_assert (ip4_addr);
	g_assert_cmpstr (nm_ip_address_get_address (ip4_addr), ==, "192.168.6.200");
	g_assert_cmpint (nm_ip_address_get_prefix (ip4_addr), ==, 24);

	g_assert_cmpstr (nm_setting_ip_config_get_gateway (s_ip4), ==, NULL);

	g_object_unref (connection);
}

static void
test_read_ibft (void)
{
	NMConnection *connection;
	NMSettingIPConfig *s_ip4;
	NMSettingIPConfig *s_ip6;
	GError *error = NULL;

	/* This test doesn't actually test too much (apart from the presence of
	 * IPv6 that is not covered by other tests), but the test fixture is a good
	 * example of about everything that can be included in iBFT table (as of
	 * ACPI 3.0b). */

	connection = read_connection (TEST_INITRD_DIR "/sysfs", "00:53:00:AB:00:01", &error);
	g_assert (connection);
	g_assert_no_error (error);

	s_ip4 = nm_connection_get_setting_ip4_config (connection);
	nmtst_assert_connection_verifies_without_normalization (connection);
	g_assert (nm_setting_ip_config_get_num_addresses (s_ip4) == 0);
	g_assert_cmpstr (nm_setting_ip_config_get_method (s_ip4), ==, NM_SETTING_IP4_CONFIG_METHOD_DISABLED);

	s_ip6 = nm_connection_get_setting_ip6_config (connection);
	g_assert (nm_setting_ip_config_get_num_addresses (s_ip6) == 1);
	g_assert_cmpstr (nm_setting_ip_config_get_method (s_ip6), ==, NM_SETTING_IP6_CONFIG_METHOD_AUTO);
	g_object_unref (connection);

	connection = read_connection (TEST_INITRD_DIR "/sysfs", "00:53:06:66:AB:01", &error);
	g_assert (connection);
	g_assert_no_error (error);
	nmtst_assert_connection_verifies_without_normalization (connection);

	s_ip4 = nm_connection_get_setting_ip4_config (connection);
	g_assert (nm_setting_ip_config_get_num_addresses (s_ip4) == 1);
	g_assert_cmpstr (nm_setting_ip_config_get_method (s_ip4), ==, NM_SETTING_IP4_CONFIG_METHOD_AUTO);

	s_ip6 = nm_connection_get_setting_ip6_config (connection);
	g_assert (nm_setting_ip_config_get_num_addresses (s_ip6) == 0);
	g_assert_cmpstr (nm_setting_ip_config_get_method (s_ip6), ==, NM_SETTING_IP6_CONFIG_METHOD_DISABLED);
	g_object_unref (connection);
}
#endif

NMTST_DEFINE ();

static void
test_read_dt_ofw (void)
{
	NMConnection *connection;
	NMSettingConnection *s_con;
	NMSettingWired *s_wired;
	NMSettingIPConfig *s_ip4;
	NMSettingIPConfig *s_ip6;
	const char *mac_address;

	connection = nmi_dt_reader_parse (TEST_INITRD_DIR "/sysfs-dt");
	g_assert (connection);
	nmtst_assert_connection_verifies (connection);

	s_con = nm_connection_get_setting_connection (connection);
	g_assert (s_con);
	g_assert_cmpstr (nm_setting_connection_get_connection_type (s_con), ==, NM_SETTING_WIRED_SETTING_NAME);
	g_assert_cmpstr (nm_setting_connection_get_id (s_con), ==, "OpenFirmware Connection");
	g_assert_cmpint (nm_setting_connection_get_timestamp (s_con), ==, 0);
	g_assert (nm_setting_connection_get_autoconnect (s_con));

	s_wired = nm_connection_get_setting_wired (connection);
	g_assert (s_wired);
	mac_address = nm_setting_wired_get_mac_address (s_wired);
	g_assert (mac_address);
	g_assert (nm_utils_hwaddr_matches (mac_address, -1, "ac:7f:3e:e5:d8:d8", -1));
	g_assert (!nm_setting_wired_get_duplex (s_wired));
	g_assert_cmpint (nm_setting_wired_get_speed (s_wired), ==, 0);
	g_assert_cmpint (nm_setting_wired_get_mtu (s_wired), ==, 0);

	s_ip4 = nm_connection_get_setting_ip4_config (connection);
	g_assert (s_ip4);
	g_assert_cmpstr (nm_setting_ip_config_get_method (s_ip4), ==, NM_SETTING_IP4_CONFIG_METHOD_AUTO);
	g_assert_cmpstr (nm_setting_ip_config_get_dhcp_hostname (s_ip4), ==, "demiurge");

	s_ip6 = nm_connection_get_setting_ip6_config (connection);
	g_assert (s_ip6);
	g_assert_cmpstr (nm_setting_ip_config_get_method (s_ip6), ==, NM_SETTING_IP6_CONFIG_METHOD_AUTO);

	g_object_unref (connection);
}

static void
test_read_dt_slof (void)
{
	NMConnection *connection;
	NMSettingConnection *s_con;
	NMSettingWired *s_wired;
	NMSettingIPConfig *s_ip4;
	NMSettingIPConfig *s_ip6;
	NMIPAddress *ip4_addr;

	connection = nmi_dt_reader_parse (TEST_INITRD_DIR "/sysfs-dt-tftp");
	g_assert (connection);
	nmtst_assert_connection_verifies (connection);

	s_con = nm_connection_get_setting_connection (connection);
	g_assert (s_con);
	g_assert_cmpstr (nm_setting_connection_get_connection_type (s_con), ==, NM_SETTING_WIRED_SETTING_NAME);
	g_assert_cmpstr (nm_setting_connection_get_id (s_con), ==, "OpenFirmware Connection");
	g_assert_cmpint (nm_setting_connection_get_timestamp (s_con), ==, 0);
	g_assert (nm_setting_connection_get_autoconnect (s_con));

	s_wired = nm_connection_get_setting_wired (connection);
	g_assert (s_wired);
	g_assert (!nm_setting_wired_get_mac_address (s_wired));
	g_assert_cmpstr (nm_setting_wired_get_duplex (s_wired), ==, "half");
	g_assert_cmpint (nm_setting_wired_get_speed (s_wired), ==, 10);
	g_assert_cmpint (nm_setting_wired_get_mtu (s_wired), ==, 0);

	s_ip4 = nm_connection_get_setting_ip4_config (connection);
	g_assert (s_ip4);
	g_assert_cmpstr (nm_setting_ip_config_get_method (s_ip4), ==, NM_SETTING_IP4_CONFIG_METHOD_MANUAL);

	g_assert_cmpint (nm_setting_ip_config_get_num_addresses (s_ip4), ==, 1);
	ip4_addr = nm_setting_ip_config_get_address (s_ip4, 0);
	g_assert (ip4_addr);
	g_assert_cmpstr (nm_ip_address_get_address (ip4_addr), ==, "192.168.32.2");
	g_assert_cmpint (nm_ip_address_get_prefix (ip4_addr), ==, 16);

	g_assert_cmpstr (nm_setting_ip_config_get_gateway (s_ip4), ==, "192.168.32.1");

	s_ip6 = nm_connection_get_setting_ip6_config (connection);
	g_assert (s_ip6);
	g_assert_cmpstr (nm_setting_ip_config_get_method (s_ip6), ==, NM_SETTING_IP6_CONFIG_METHOD_DISABLED);

	g_object_unref (connection);
}

static void
test_read_dt_none (void)
{
	NMConnection *connection;

	connection = nmi_dt_reader_parse (TEST_INITRD_DIR "/sysfs");
	g_assert (!connection);
}

int main (int argc, char **argv)
{
	nmtst_init_assert_logging (&argc, &argv, "INFO", "DEFAULT");

	g_test_add_func ("/initrd/dt/ofw", test_read_dt_ofw);
	g_test_add_func ("/initrd/dt/slof", test_read_dt_slof);
	g_test_add_func ("/initrd/dt/none", test_read_dt_none);

	return g_test_run ();
}