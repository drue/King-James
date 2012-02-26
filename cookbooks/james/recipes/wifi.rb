package "bridge-utils"
package "iptables"
package "dhcp3-server"
package "hostapd"

cookbook_file "/etc/network/interfaces" do
  source "interfaces"
  mode "0644"
end

cookbook_file "/etc/sysctl.conf" do
  source "sysctl.conf"
  mode "0644"
end

cookbook_file "/etc/iptables.rules" do
  source "iptables.rules"
  mode "0644"
end

cookbook_file "/etc/dhcp/dhcpd.conf" do
  source "dhcpd.conf"
  mode "0644"
end

cookbook_file "/etc/default/dhcp3-server" do
  source "dhcp3-server"
  mode "0644"
end

cookbook_file "/etc/hostapd/hostapd.conf" do
  source "hostapd.conf"
  mode "0600"
end

cookbook_file "/etc/default/hostapd" do
  source "hostapd"
  mode "0644"
end

