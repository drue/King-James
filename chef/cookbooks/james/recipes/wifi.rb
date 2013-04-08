include_recipe "apt"

package "hostapd"
package "dnsmasq"

service "hostapd"
service "dnsmasq"

cookbook_file "/etc/network/interfaces" do
  mode "0644"
end

cookbook_file "/etc/default/hostapd" do
  mode "0644"
end

cookbook_file "/etc/hostapd/hostapd.conf" do
  mode "0600"
  notifies :restart, "service[hostapd]"
end


cookbook_file "/etc/dnsmasq.conf" do
  mode "0600"
  notifies :restart, "service[dnsmasq]"
end
