object fmNumistaSettings: TfmNumistaSettings
  Left = 0
  Top = 0
  BorderStyle = bsDialog
  Caption = 'Numista'
  ClientHeight = 290
  ClientWidth = 520
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  Position = poOwnerFormCenter
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object lblApiKey: TLabel
    Left = 12
    Top = 16
    Width = 44
    Height = 13
    Caption = 'API &key:'
    FocusControl = edApiKey
  end
  object lblStatus: TLabel
    Left = 110
    Top = 78
    Width = 398
    Height = 42
    AutoSize = False
    Caption = 'Not connected.'
    ShowAccelChar = False
    WordWrap = True
  end
  object lblRequests: TLabel
    Left = 110
    Top = 128
    Width = 398
    Height = 13
    AutoSize = False
    Caption = 'Requests this month: 0 of 2000'
  end
  object lblLists: TLabel
    Left = 110
    Top = 158
    Width = 398
    Height = 13
    AutoSize = False
    Caption = 'Numista lists are not loaded yet.'
  end
  object lblDocs: TLabel
    Left = 110
    Top = 222
    Width = 104
    Height = 13
    Cursor = crHandPoint
    Caption = 'Numista API documentation'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clHotLight
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsUnderline]
    ParentFont = False
    OnClick = lblDocsClick
  end
  object edApiKey: TEdit
    Left = 110
    Top = 13
    Width = 300
    Height = 21
    PasswordChar = '*'
    TabOrder = 0
  end
  object chkShowKey: TCheckBox
    Left = 420
    Top = 15
    Width = 90
    Height = 17
    Caption = '&Show'
    TabOrder = 1
    OnClick = chkShowKeyClick
  end
  object btnCheck: TButton
    Left = 110
    Top = 44
    Width = 170
    Height = 25
    Caption = '&Check connection'
    TabOrder = 2
    OnClick = btnCheckClick
  end
  object btnRefreshLists: TButton
    Left = 110
    Top = 180
    Width = 170
    Height = 25
    Caption = '&Refresh Numista lists'
    TabOrder = 3
    OnClick = btnRefreshListsClick
  end
  object btnOK: TButton
    Left = 352
    Top = 254
    Width = 75
    Height = 25
    Caption = 'OK'
    Default = True
    ModalResult = 1
    TabOrder = 4
  end
  object btnCancel: TButton
    Left = 433
    Top = 254
    Width = 75
    Height = 25
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 5
  end
end
