include_recipe "apt"

#package "hostapd"

bash "depmod" do
  code "depmod -a"
  action :nothing
end


cookbook_file "/usr/sbin/hostapd" do
  mode "0755"
  path "hostapd.bin"
end

# cookbook_file "/lib/modules/3.2.0-40-lowlatency/kernel/drivers/net/wireless/8192cu.ko" do
#   mode "0755"
#   notifies :run, "bash[depmod]"
# end

package "dnsmasq"

service "hostapd" do
  provider Chef::Provider::Service::Init::Debian
end

service "dnsmasq" do
  provider Chef::Provider::Service::Init::Debian
end

cookbook_file "/etc/network/interfaces" do
  mode "0644"
end

cookbook_file "/etc/default/hostapd" do
  mode "0644"
end

directory "/etc/hostapd" do
  mode 0644
end

cookbook_file "/etc/hostapd/hostapd.conf" do
  mode "0600"
  notifies :restart, "service[hostapd]"
end


cookbook_file "/etc/dnsmasq.conf" do
  mode "0600"
  notifies :restart, "service[dnsmasq]"
end
