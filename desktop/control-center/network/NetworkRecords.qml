// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.networkmanagement as PlasmaNM
QtObject {
    id:records
    required property var connectionModel
    property var entries:[]
    function refresh() {
        let result=[];
        for(let i=0;i<items.count;i++){const item=items.objectAt(i);if(item) result.push(item.snapshot);}
        entries=result;
    }
    property Instantiator items:Instantiator {
        id:items;model:records.connectionModel
        delegate:QtObject {
            required property var model
            required property int index
            readonly property var snapshot:({row:index,section:model.Section || "",name:model.ItemUniqueName || "",icon:model.ConnectionIcon || "network-wireless-symbolic",active:model.ConnectionState===PlasmaNM.Enums.Activated,connecting:model.ConnectionState===PlasmaNM.Enums.Activating,type:model.Type,signal:model.Signal || 0,security:model.SecurityTypeString || "",securityKnown:model.SecurityType!==PlasmaNM.Enums.UnknownSecurity,secured:[PlasmaNM.Enums.StaticWep,PlasmaNM.Enums.DynamicWep,PlasmaNM.Enums.Leap,PlasmaNM.Enums.WpaPsk,PlasmaNM.Enums.WpaEap,PlasmaNM.Enums.Wpa2Psk,PlasmaNM.Enums.Wpa2Eap,PlasmaNM.Enums.SAE,PlasmaNM.Enums.Wpa3SuiteB192].includes(model.SecurityType),saved:!!model.Uuid,connectionPath:model.ConnectionPath,devicePath:model.DevicePath,specificPath:model.SpecificPath})
            onSnapshotChanged:Qt.callLater(records.refresh)
        }
        onObjectAdded:Qt.callLater(records.refresh)
        onObjectRemoved:Qt.callLater(records.refresh)
    }
}
