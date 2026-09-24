object dmData: TdmData
  OldCreateOrder = False
  Height = 251
  Width = 415
  object FDConnection: TFDConnection
    Params.Strings = (
      'DriverID=SQLite'
      'ForeignKeys=On'
      'LockingMode=Normal')
    LoginPrompt = False
    Transaction = FDTransaction
    Left = 56
    Top = 32
  end
  object FDTransaction: TFDTransaction
    Connection = FDConnection
    Left = 56
    Top = 96
  end
  object FDQuery: TFDQuery
    Connection = FDConnection
    Left = 56
    Top = 160
  end
  object FDPhysSQLiteDriverLink: TFDPhysSQLiteDriverLink
    EngineLinkage = slStatic
    Left = 200
    Top = 32
  end
  object FDGUIxWaitCursor: TFDGUIxWaitCursor
    Provider = 'Forms'
    Left = 200
    Top = 96
  end
end
