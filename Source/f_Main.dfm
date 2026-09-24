object fmMain: TfmMain
  Left = 0
  Top = 0
  Caption = 'Coins Collection'
  ClientHeight = 521
  ClientWidth = 884
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  Menu = MainMenu
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 13
  object ToolBar: TToolBar
    Left = 0
    Top = 0
    Width = 884
    Height = 22
    AutoSize = True
    ButtonHeight = 21
    ButtonWidth = 50
    Caption = 'ToolBar'
    List = True
    ShowCaptions = True
    TabOrder = 0
    object tbAdd: TToolButton
      Left = 0
      Top = 0
      Action = actAdd
      AutoSize = True
    end
    object tbEdit: TToolButton
      Left = 50
      Top = 0
      Action = actEdit
      AutoSize = True
    end
    object tbDelete: TToolButton
      Left = 100
      Top = 0
      Action = actDelete
      AutoSize = True
    end
    object tbSep1: TToolButton
      Left = 150
      Top = 0
      Width = 8
      Style = tbsSeparator
    end
    object tbImport: TToolButton
      Left = 158
      Top = 0
      Action = actImport
      AutoSize = True
      Caption = 'Import'
    end
  end
  object lvItems: TListView
    Left = 0
    Top = 22
    Width = 884
    Height = 480
    Align = alClient
    Columns = <
      item
        Caption = 'Period'
        Width = 200
      end
      item
        Caption = 'Currency'
        Width = 110
      end
      item
        Caption = 'Denomination'
        Width = 160
      end
      item
        Alignment = taRightJustify
        Caption = 'Year'
        Width = 60
      end
      item
        Caption = 'Condition'
        Width = 75
      end
      item
        Caption = 'Number'
        Width = 120
      end
      item
        Alignment = taRightJustify
        Caption = 'Value'
        Width = 90
      end>
    HideSelection = False
    MultiSelect = True
    OwnerData = True
    ReadOnly = True
    RowSelect = True
    TabOrder = 1
    ViewStyle = vsReport
    OnColumnClick = lvItemsColumnClick
    OnData = lvItemsData
    OnDblClick = lvItemsDblClick
    OnKeyDown = lvItemsKeyDown
  end
  object StatusBar: TStatusBar
    Left = 0
    Top = 502
    Width = 884
    Height = 19
    Panels = <
      item
        Width = 200
      end>
  end
  object ActionList: TActionList
    Left = 344
    Top = 96
    object actImport: TAction
      Category = 'File'
      Caption = '&Import from uCoin (.xlsx)...'
      Enabled = False
    end
    object actBackup: TAction
      Category = 'File'
      Caption = '&Backup database...'
      Enabled = False
    end
    object actExit: TAction
      Category = 'File'
      Caption = 'E&xit'
      ShortCut = 32883
      OnExecute = actExitExecute
    end
    object actAdd: TAction
      Category = 'Collection'
      Caption = '&Add'
      Enabled = False
      ShortCut = 45
    end
    object actEdit: TAction
      Category = 'Collection'
      Caption = '&Edit'
      Enabled = False
    end
    object actDelete: TAction
      Category = 'Collection'
      Caption = '&Delete'
      Enabled = False
      ShortCut = 46
    end
    object actNeedToReplace: TAction
      Category = 'Collection'
      AutoCheck = True
      Caption = '&Need to replace'
      Enabled = False
    end
    object actRefresh: TAction
      Category = 'Collection'
      Caption = '&Refresh'
      ShortCut = 116
      OnExecute = actRefreshExecute
    end
    object actCountries: TAction
      Category = 'Reference'
      Caption = '&Countries...'
      Enabled = False
    end
    object actPeriods: TAction
      Category = 'Reference'
      Caption = '&Periods...'
      Enabled = False
    end
    object actCurrencies: TAction
      Category = 'Reference'
      Caption = 'C&urrencies...'
      Enabled = False
    end
    object actNumistaSettings: TAction
      Category = 'Numista'
      Caption = '&Connect / Settings...'
      Enabled = False
    end
    object actNumistaUploadSelected: TAction
      Category = 'Numista'
      Caption = 'Upload &selected'
      Enabled = False
    end
    object actNumistaUploadAll: TAction
      Category = 'Numista'
      Caption = 'Upload &all'
      Enabled = False
    end
    object actSettings: TAction
      Category = 'Settings'
      Caption = '&Settings...'
      Enabled = False
    end
    object actAbout: TAction
      Category = 'Help'
      Caption = '&About'
      ShortCut = 112
      OnExecute = actAboutExecute
    end
  end
  object MainMenu: TMainMenu
    Left = 424
    Top = 96
    object miFile: TMenuItem
      Caption = '&File'
      object miImport: TMenuItem
        Action = actImport
      end
      object miBackup: TMenuItem
        Action = actBackup
      end
      object miFileSep1: TMenuItem
        Caption = '-'
      end
      object miExit: TMenuItem
        Action = actExit
      end
    end
    object miCollection: TMenuItem
      Caption = '&Collection'
      object miAdd: TMenuItem
        Action = actAdd
      end
      object miEdit: TMenuItem
        Action = actEdit
        ShortCut = 13
      end
      object miDelete: TMenuItem
        Action = actDelete
      end
      object miCollectionSep1: TMenuItem
        Caption = '-'
      end
      object miNeedToReplace: TMenuItem
        Action = actNeedToReplace
        AutoCheck = True
      end
      object miCollectionSep2: TMenuItem
        Caption = '-'
      end
      object miRefresh: TMenuItem
        Action = actRefresh
      end
    end
    object miReference: TMenuItem
      Caption = '&Reference'
      object miCountries: TMenuItem
        Action = actCountries
      end
      object miPeriods: TMenuItem
        Action = actPeriods
      end
      object miCurrencies: TMenuItem
        Action = actCurrencies
      end
    end
    object miNumista: TMenuItem
      Caption = '&Numista'
      object miNumistaSettings: TMenuItem
        Action = actNumistaSettings
      end
      object miNumistaSep1: TMenuItem
        Caption = '-'
      end
      object miNumistaUploadSelected: TMenuItem
        Action = actNumistaUploadSelected
      end
      object miNumistaUploadAll: TMenuItem
        Action = actNumistaUploadAll
      end
    end
    object miSettings: TMenuItem
      Caption = '&Settings'
      object miLanguage: TMenuItem
        Caption = '&Language'
      end
      object miSettingsDialog: TMenuItem
        Action = actSettings
      end
    end
    object miHelp: TMenuItem
      Caption = '&Help'
      object miAbout: TMenuItem
        Action = actAbout
      end
    end
  end
end
