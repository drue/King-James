action :sync do
  execute "clone repository" do    
    not_if "hg identify #{new_resource.path}"
    command "hg clone -e 'ssh -i #{new_resource.key} -o StrictHostKeyChecking=no' -r #{new_resource.reference} #{new_resource.repository} #{new_resource.path}"
  end
  execute "pull changes" do
      command "cd #{new_resource.path} && hg pull -e 'ssh -i #{new_resource.key} -o StrictHostKeyChecking=no' -r #{new_resource.reference} #{new_resource.repository}"
  end
  execute "update" do
      command "cd #{new_resource.path} && hg update -r #{new_resource.reference}"
  end
  execute "update owner" do
    command "chown -R #{new_resource.owner}:#{new_resource.group} #{new_resource.path}"
  end
  execute "update permissions" do
    command "chmod -R #{new_resource.mode} #{new_resource.path}"
  end
end
 
action :clone do
  execute "clone repository" do
    not_if "hg identify #{new_resource.path}"
    command "hg clone -e 'ssh -i #{new_resource.key} -o StrictHostKeyChecking=no' -r #{new_resource.reference} #{new_resource.repository} #{new_resource.path}"
  end
  execute "update owner" do
    command "chown -R #{new_resource.owner}:#{new_resource.group} #{new_resource.path}"
  end
  execute "update permissions" do
    command "chmod -R #{new_resource.mode} #{new_resource.path}"
  end
end
