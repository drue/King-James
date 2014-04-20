name "deputy"
description "Burris Ewell's BitBucket"
run_list(
         "recipe[james::james]",
         "recipe[james::alsactl]",
         "recipe[james::wifi]",
         "recipe[james::lighttpd]",
         "recipe[james::sensors]",
         "recipe[james::server]"
         )
