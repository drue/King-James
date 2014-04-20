package "alsa"
package "python-pip"
package "python-dev"
package "python2.7-dev"
package "libzmq-dev"
package "libboost-dev"
package "libboost-thread-dev"

python_pip "pyzmq" do
  action :install
end

python_pip "tornado" do
  action :install
end

python_pip "git+https://github.com/mrjoes/tornadio2.git" do
  action :install
end

directory "/etc/james"

cookbook_file "/etc/james/james.conf" do
  action :create
end

service "james" do
  provider Chef::Provider::Service::Init::Debian
  supports :status => true, :restart => true, :reload => true
  action [ :enable, :start ]
end

