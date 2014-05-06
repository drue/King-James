package "cpufreqd"
package "cpufrequtils"

service "cpufreqd" do
  action :enable
end

cookbook_file "/etc/cpufreqd.conf" do
  notifies :restart, "service[cpufreqd]"
end
