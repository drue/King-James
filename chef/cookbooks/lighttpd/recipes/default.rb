package "lighttpd"

template "/etc/lighttpd/lighttpd.conf"

service "lighttpd" do
  action [ :enable, :start ]
end

