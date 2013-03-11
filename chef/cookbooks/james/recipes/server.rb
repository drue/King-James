directory "/var/audio" do
  mode "0755"
end

cookbook_file "/etc/init/james.conf" do
  source "upstart.conf"
end

service "james" do
  provider Chef::Provider::Service::Upstart
  supports :status => true, :restart => true, :reload => true
  action [ :enable, :start ]
end
  
