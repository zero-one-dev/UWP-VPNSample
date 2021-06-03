using System;
using Windows.Networking.Vpn;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace VPNSample
{


    public sealed partial class MainPage : Page
    {
        private string vpn_profile_name = "My_VPNSample";

        public MainPage()
        {
            this.InitializeComponent();
        }





        private async void ConnectButton_Click(object sender, RoutedEventArgs e)
        {
            VpnPlugInProfile profile = new VpnPlugInProfile();
            profile.ProfileName = vpn_profile_name;
            VpnManagementAgent agent = new VpnManagementAgent();
            await agent.DisconnectProfileAsync(profile);



            profile.CustomConfiguration = $"ipv4={TextBoxLanIp.Text}&port={TextBoxPort.Text}";
            profile.AlwaysOn = true;
            profile.RememberCredentials = true;
            profile.RequireVpnClientAppUI = true;
            profile.VpnPluginPackageFamilyName = Windows.ApplicationModel.Package.Current.Id.FamilyName;

            String serverUri = "http://" + TextBoxIP.Text;
            profile.ServerUris.Add(new Uri(serverUri));

            VpnManagementErrorStatus udpate_result = await agent.UpdateProfileFromObjectAsync(profile);
            VpnManagementErrorStatus connect_result = VpnManagementErrorStatus.Ok;
            if (udpate_result == VpnManagementErrorStatus.Ok)
            {
                connect_result = await agent.ConnectProfileAsync(profile);
                if (connect_result == VpnManagementErrorStatus.Ok)
                {
                    return;
                }
            }
        }

        private async void DisConnectButton_Click(object sender, RoutedEventArgs e)
        {
            VpnPlugInProfile profile = new VpnPlugInProfile { ProfileName = vpn_profile_name };
            VpnManagementAgent agent = new VpnManagementAgent();
            await agent.DisconnectProfileAsync(profile);
        }
    }
}
