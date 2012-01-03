 template "/etc/apt/apt.conf" do
   source "apt.conf.erb"
   variables(
             :proxy => '"http://10.0.2.2:3128/"'
             )
 end
