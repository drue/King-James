include_recipe "apt"

package "hostapd"

# bash "depmod" do
#   code "depmod -a"
#   action :nothing
# end


cookbook_file "/usr/sbin/hostapd" do
  mode "0755"
  source "hostapd.armhf.bin"
end

# cookbook_file "/lib/modules/3.2.0-40-lowlatency/kernel/drivers/net/wireless/8192cu.ko" do
#   mode "0755"
#   notifies :run, "bash[depmod]"
# end

package "dnsmasq" do
  action :remove
end

# cookbook_file "/etc/dnsmasq.conf" do
#   mode "0600"
#   notifies :restart, "service[dnsmasq]"
# end


package "udhcpd"

cookbook_file "/etc/udhcpd.conf" do
  mode 0644
  notifies :restart, "service[udhcpd]"
end

cookbook_file "/etc/default/udhcpd" do
  source "udhcpd.default"
  mode 0644
end

cookbook_file "/etc/default/hostapd" do
  mode "0644"
end

cookbook_file "/etc/network/interfaces" do
  mode "0644"
end


directory "/etc/hostapd" do
  mode 0644
end

cookbook_file "/etc/hostapd/hostapd.conf" do
  mode "0600"
  notifies :restart, "service[hostapd]"
end

service "udhcpd" do
  action :disable
  provider Chef::Provider::Service::Init::Debian
end

service "hostapd" do
  provider Chef::Provider::Service::Init::Debian
  action :disable
end

cookbook_file "/etc/init/wifi.conf" do
  source "wifi.upstart"
end

service "wifi" do
  provider Chef::Provider::Service::Upstart
  action :enable
end

# service "dnsmasq" do
#   provider Chef::Provider::Service::Init::Debian
#   action [ :enable, :start ]
# end

