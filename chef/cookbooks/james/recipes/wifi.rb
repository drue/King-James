package "bridge-utils"
package "iptables"
package "dhcp3-server"
package "hostapd"

cookbook_file "/etc/network/interfaces" do
  mode "0644"
end

cookbook_file "/etc/sysctl.conf" do
  mode "0644"
end

cookbook_file "/etc/iptables.rules" do
  mode "0644"
end

cookbook_file "/etc/dhcp/dhcpd.conf" do
  mode "0644"
end

cookbook_file "/etc/default/dhcp3-server" do
  mode "0644"
end

cookbook_file "/etc/hostapd/hostapd.conf" do
  mode "0600"
end

cookbook_file "/etc/default/hostapd" do
  mode "0644"
end

