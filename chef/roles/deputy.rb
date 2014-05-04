name "deputy"
description "Burris Ewell's BitBucket"
run_list(
         "recipe[james::james]",
         "recipe[james::wifi]",
         "recipe[james::lighttpd]",
         "recipe[james::server]"
         )
