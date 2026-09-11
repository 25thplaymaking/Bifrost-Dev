AudioSignalResClass {
 Inputs {
  IOPItemInputClass {
   id 1
   name "BifrostGain"
   children {
    2
   }
   value 1
   valueMax 2
  }
  IOPItemInputClass {
   id 3
   name "BifrostLowpass"
   children {
    4
   }
   value 20000
   valueMax 20000
  }
  IOPItemInputClass {
   id 5
   name "BifrostReverb"
   children {
    15 17 19 21
   }
   value 0.3
  }
  IOPItemInputClass {
   id 7
   name "SmallWet"
   children {
    15
   }
  }
  IOPItemInputClass {
   id 9
   name "MediumWet"
   children {
    17
   }
  }
  IOPItemInputClass {
   id 11
   name "LargeWet"
   children {
    19
   }
  }
  IOPItemInputClass {
   id 13
   name "ExteriorWet"
   children {
    21
   }
  }
 }
 Ops {
  IOPItemOpMulClass {
   id 15
   name "Wet0"
   children {
    16
   }
   inputs {
    ConnectionClass "7:0" {
     id 7
     port 0
    }
    ConnectionClass "5:0" {
     id 5
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 17
   name "Wet1"
   children {
    18
   }
   inputs {
    ConnectionClass "9:0" {
     id 9
     port 0
    }
    ConnectionClass "5:0" {
     id 5
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 19
   name "Wet2"
   children {
    20
   }
   inputs {
    ConnectionClass "11:0" {
     id 11
     port 0
    }
    ConnectionClass "5:0" {
     id 5
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 21
   name "Wet3"
   children {
    22
   }
   inputs {
    ConnectionClass "13:0" {
     id 13
     port 0
    }
    ConnectionClass "5:0" {
     id 5
     port 0
    }
   }
  }
 }
 Outputs {
  IOPItemOutputClass {
   id 2
   name "Gain"
   input 1
  }
  IOPItemOutputClass {
   id 4
   name "Lowpass"
   input 3
  }
  IOPItemOutputClass {
   id 16
   name "Wet0"
   input 15
  }
  IOPItemOutputClass {
   id 18
   name "Wet1"
   input 17
  }
  IOPItemOutputClass {
   id 20
   name "Wet2"
   input 19
  }
  IOPItemOutputClass {
   id 22
   name "ExteriorReverb"
   tl 192 192
   input 21
  }
 }
 Input_Order {
  ItemDetailListItemClass BifrostGain {
   Name "BifrostGain"
   Id 1
  }
  ItemDetailListItemClass BifrostLowpass {
   Name "BifrostLowpass"
   Id 3
  }
  ItemDetailListItemClass BifrostReverb {
   Name "BifrostReverb"
   Id 5
  }
  ItemDetailListItemClass SmallWet {
   Name "SmallWet"
   Id 7
  }
  ItemDetailListItemClass MediumWet {
   Name "MediumWet"
   Id 9
  }
  ItemDetailListItemClass LargeWet {
   Name "LargeWet"
   Id 11
  }
  ItemDetailListItemClass ExteriorWet {
   Name "ExteriorWet"
   Id 13
  }
 }
 Output_Order {
  ItemDetailListItemClass Gain {
   Name "Gain"
   Id 2
  }
  ItemDetailListItemClass Lowpass {
   Name "Lowpass"
   Id 4
  }
  ItemDetailListItemClass Wet0 {
   Name "Wet0"
   Id 16
  }
  ItemDetailListItemClass Wet1 {
   Name "Wet1"
   Id 18
  }
  ItemDetailListItemClass Wet2 {
   Name "Wet2"
   Id 20
  }
  ItemDetailListItemClass Wet3 {
   Name "Wet3"
   Id 22
  }
 }
}