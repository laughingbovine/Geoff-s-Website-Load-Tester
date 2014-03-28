#!/opt/local/bin/bash

SERVERS='webcache-dev1.cc.columbia.edu'
#SERVERS='cuit-wordpress-test1.cc.columbia.edu cuit-wordpress-test2.cc.columbia.edu'
#SERVERS='cuit-wordpress-test1.cc.columbia.edu cuit-wordpress-test2.cc.columbia.edu test3-blogs.cuit.columbia.edu'
#SERVERS='cuit-wordpress-test.cc.columbia.edu test3-blogs.cuit.columbia.edu'
SERVER_PORT='80'

#REQUESTS='-i requests/wordpress_cmts -i requests/wordpress_coop -i requests/wordpress_crfcfw_test3 -i requests/wordpress_equestrian -i requests/wordpress_homepage -i requests/wordpress_medren_test3 -i requests/wordpress_sdds -i requests/wordpress_sipa -i requests/wordpress_vals -i requests/wordpress_visualarts'

REQUESTS='-i requests/wordpress_cmts -i requests/wordpress_coop -i requests/wordpress_equestrian -i requests/wordpress_homepage -i requests/wordpress_sdds -i requests/wordpress_sipa -i requests/wordpress_vals -i requests/wordpress_visualarts'

CONCURRENT_USERS='10'
RAMP_FACTOR='24'
RAMP_WAIT='5'

#USER_SESSIONS='100'
USER_MAXTIME='600'

for SERVER in $SERVERS
do
    #echo $SERVER;
    date;
    #time ./loadtest -h $SERVER -p $SERVER_PORT -c $CONCURRENT_USERS -r $USER_SESSIONS -t $USER_MAXTIME -R $RAMP_FACTOR -W $RAMP_WAIT $REQUESTS
    time ./loadtest -h $SERVER -p $SERVER_PORT -c $CONCURRENT_USERS -t $USER_MAXTIME -R $RAMP_FACTOR -W $RAMP_WAIT $REQUESTS
done
