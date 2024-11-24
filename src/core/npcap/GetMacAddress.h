#ifndef __GET_MAC_ADDRESS_H
#define __GET_MAC_ADDRESS_H

#include <stdint.h>
#include <string>

/**
 * @brief ʹ��Packet���ȡ����������MAC��ַ
 * ��д�ԣ�\npcap-sdk-1.13\Examples-remote\PacketDriver\GetMacAddress
 *
 * @param mac	[out]��ȡ��MAC��ַ
 * @param name	[in]pcap_findalldevs_ex��ȡ����������
 * @return int	�ɹ�����0��ʧ�ܷ���-1
 */
int GetMac(uint8_t mac[], std::string name);

#endif//__GET_MAC_ADDRESS_H