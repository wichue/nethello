#ifndef __GET_MAC_ADDRESS_H
#define __GET_MAC_ADDRESS_H

#include <stdint.h>
#include <string>

/**
 * @brief 使用Packet库获取本地网卡的MAC地址
 * 改写自：\npcap-sdk-1.13\Examples-remote\PacketDriver\GetMacAddress
 *
 * @param mac	[out]获取的MAC地址
 * @param name	[in]pcap_findalldevs_ex获取的网卡名称
 * @return int	成功返回0，失败返回-1
 */
int GetMac(uint8_t mac[], std::string name);

#endif//__GET_MAC_ADDRESS_H