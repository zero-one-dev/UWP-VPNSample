#pragma once

namespace winrt::VpnDll::implementation
{
	struct VpnPlugin : implements<VpnPlugin, winrt::Windows::Networking::Vpn::IVpnPlugIn>
	{
		VpnPlugin() = default;

		void Connect(Windows::Networking::Vpn::VpnChannel const& channel);
		void Disconnect(Windows::Networking::Vpn::VpnChannel const& channel);
		void GetKeepAlivePayload(Windows::Networking::Vpn::VpnChannel const& channel, Windows::Networking::Vpn::VpnPacketBuffer& keepAlivePacket);
		void Encapsulate(Windows::Networking::Vpn::VpnChannel const& channel, Windows::Networking::Vpn::VpnPacketBufferList const& packets, Windows::Networking::Vpn::VpnPacketBufferList const& encapulatedPackets);
		void Decapsulate(Windows::Networking::Vpn::VpnChannel const& channel, Windows::Networking::Vpn::VpnPacketBuffer const& encapBuffer, Windows::Networking::Vpn::VpnPacketBufferList const& decapsulatedPackets, Windows::Networking::Vpn::VpnPacketBufferList const& controlPacketsToSend);

	private:		
		void Configure(Windows::Networking::Vpn::VpnChannel const& channel, std::wstring ipv4);	
	};

}
