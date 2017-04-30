gcc sensor_hubs.cpp -o sensor_hubs -lstdc++ -lpthread -lm

gcc display_heart.cpp -o display_heart -lstdc++ -lpthread
gcc display_location.cpp -o display_location -lstdc++ -lpthread
gcc display_oxygen.cpp -o display_oxygen -lstdc++ -lpthread
gcc display_toxic.cpp -o display_toxic -lstdc++ -lpthread
gnome-terminal -e "./sensor_hubs"
gcc first_responder.cpp -o first_responder -lstdc++ -lpthread -lm
gcc fire_chief.cpp -o fire_chief -lstdc++ -lpthread -lm

gcc changeLocations.cpp -o changeLocations -lstdc++

gnome-terminal -e "./changeLocations" -t "changing locations"


# gnome-terminal -e "./fire_chief" -t "fire_chief"

gnome-terminal -e "./display_toxic" -t "toxic" --geometry="24x30"
gnome-terminal -e "./display_oxygen" -t "oxygen" --geometry="24x30"
gnome-terminal -e "./display_location" -t "location" --geometry="24x30"
gnome-terminal -e "./display_heart" -t "heart" --geometry="24x30"

gnome-terminal -e "./first_responder" -t "first_responder"
