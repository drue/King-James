package "libasound2-dev"
package "libflac-dev"
package "build-essential"
package "python2.7-dev"
package "python-pip"
package "git-core"
package "mercurial"
package "libzmq-dev"
package "libevent-dev"
package "gdb"
package "htop"

python_pip "pyzmq" do
  action :install
end

python_pip "tornado" do
  action :install
end

python_pip "git+https://github.com/MrJoes/tornadio2.git" do
  action :install
end



