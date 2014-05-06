

# file "/etc/init.d/chef-client" do
#   action :delete
# end


# package "linux-image-3.2.0-40-lowlatency"
# package "rtirq-init"

# service "rtirq" do
#   supports :start => true
#   action :nothing
# end

# cookbook_file "/etc/default/rtirq" do
#   notifies :start, "service[rtirq]"
# end


cookbook_file "/etc/init/failsafe.conf" do
  mode "0644"
end



directory "/var/audio" do
  mode "0755"
end

cookbook_file "/etc/init/james.conf" do
  source "upstart.conf"
  mode "0644"
end

# execute "update-grub" do
#   action :nothing
# end

# cookbook_file "/etc/grub.d/00_header" do
#   mode "0755"
#   notifies :run, "execute[update-grub]", :immediately
# end
