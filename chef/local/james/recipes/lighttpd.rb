package "lighttpd"

service "lighttpd"

cookbook_file "/etc/lighttpd/lighttpd.conf" do
  notifies :restart, "service[lighttpd]"
end


