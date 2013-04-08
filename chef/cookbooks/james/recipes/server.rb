directory "/var/audio" do
  mode "0755"
end

cookbook_file "/etc/init/james.conf" do
  source "upstart.conf"
  mode "0644"
end

execute "update-grub" do
  action :nothing
end

cookbook_file "/etc/grub.d/00_header" do
  mode "0755"
  notifies :run, "execute[update-grub]", :immediately
end

service "james" do
  provider Chef::Provider::Service::Upstart
  supports :status => true, :restart => true, :reload => true
  action [ :enable, :start ]
end
  