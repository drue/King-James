name "deputy"
description "Burris Ewell's BitBucket"
run_list(
         "recipe[james-dev]",
         "recipe[james::alsactl]",
         "recipe[james::wifi]",
         "recipe[james::cpufreq]",
         "recipe[james::lighttpd]",
         "recipe[james::sensors]",
         "recipe[james::server]"
         )
