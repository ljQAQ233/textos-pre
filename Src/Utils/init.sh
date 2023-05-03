#!/usr/bin/env bash

read -p "Press your password : " PASSWORD

echo "PASSWORD:=${PASSWORD}" > Config/Private.mk

echo "Recorded the password (${PASSWORD}) and you can change it in Src/Config/Private.mk"

echo -n "Copying udkdebugger.conf to /etc/ ..."
if ! echo ${PASSWORD} | sudo -S cp Utils/Udk/udkdebugger.conf /etc/udkdebugger.conf;then
    echo "Please exec this again! Exiting..." 
    exit 1
fi
echo "done"

echo -n "Adding PYTHONPATH into /etc/profile..."
sudo chmod 0777 /etc/profile
sudo echo -e "\nexport PYTHONPATH=\${PYTHONPATH}:$(realpath Utils/Udk/script)\n" >> /etc/profile
echo "done"
