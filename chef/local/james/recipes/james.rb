package "alsa"
package "python-pip"
package "python-dev"
package "python2.7-dev"
package "libzmq-dev"
package "libboost-dev"
package "libboost-thread-dev"
package "exfat-fuse"

package "upstart"

python_pip "pyzmq" do
  action :install
end

python_pip "tornado" do
  action :install
end

python_pip "https://github.com/mrjoes/sockjs-tornado/archive/b68096e9d30d1e37e39f6d502dbaf46f688704c3.zip" do
  action :install
end

directory "/etc/james"

cookbook_file "/etc/james/james.conf" do
  action :create_if_missing
end

cookbook_file "/etc/init/james.conf" do
  source "upstart.conf"
  action :create
  notifies :restart, "service[james]"
end

cookbook_file "/etc/init/mounta.conf" do
  source "mounta.upstart"
  action :create
end

service "james" do
  provider Chef::Provider::Service::Upstart
  supports :status => true, :restart => true, :reload => true
  action :enable
end
