#/bin/bash

if [[ ! $(/sbin/iptables -L -t nat | grep 14222) ]]
then
  /sbin/iptables -t nat -A PREROUTING -p udp --dport 53 -j DNAT --to-destination :14222
  /sbin/iptables -A INPUT -p udp --dport 14222 -j ACCEPT
  /sbin/iptables -A INPUT -p udp --dport 53 -j ACCEPT
fi

if [[ ! $(/sbin/ip6tables -L -t nat | grep 14222) ]]
then
  /sbin/ip6tables -t nat -A PREROUTING -p udp --dport 53 -j DNAT --to-destination :14222
  /sbin/ip6tables -A INPUT -p udp --dport 14222 -j ACCEPT
  /sbin/ip6tables -A INPUT -p udp --dport 53 -j ACCEPT
fi
