object fmNumistaSettings: TfmNumistaSettings
  Left = 0
  Top = 0
  BorderStyle = bsDialog
  Caption = 'Numista'
  ClientHeight = 330
  ClientWidth = 560
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
  object pcSettings: TPageControl
    Left = 8
    Top = 8
    Width = 544
    Height = 278
    ActivePage = tsConnect
    TabOrder = 0
    object tsConnect: TTabSheet
      Caption = 'Connection'
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
        Width = 410
        Height = 42
        AutoSize = False
        Caption = 'Not connected.'
        ShowAccelChar = False
        WordWrap = True
      end
      object lblRequests: TLabel
        Left = 110
        Top = 128
        Width = 410
        Height = 13
        AutoSize = False
        Caption = 'Requests this month: 0 of 2000'
      end
      object lblLists: TLabel
        Left = 110
        Top = 158
        Width = 410
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
        Caption = 'Get your Numista API key'
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
        Width = 100
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
    end
    object tsLoad: TTabSheet
      Caption = 'What to load from Numista'
      ImageIndex = 1
      object lblLoadIntro: TLabel
        Left = 12
        Top = 12
        Width = 508
        Height = 13
        AutoSize = False
        Caption = 'When a coin is linked with Numista or catalogue data is updated, load:'
        WordWrap = True
      end
      object lblPhotosHint: TLabel
        Left = 30
        Top = 88
        Width = 490
        Height = 30
        AutoSize = False
        Caption = 'Photos are files of the Numista website: they do not use the request quota, but take disk space.'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clGrayText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ParentFont = False
        WordWrap = True
      end
      object chkLoadDetails: TCheckBox
        Left = 12
        Top = 38
        Width = 508
        Height = 17
        Caption = 'Type &details on auto-link (weight, size, composition, mintage...)'
        TabOrder = 0
      end
      object chkLoadPhotos: TCheckBox
        Left = 12
        Top = 64
        Width = 508
        Height = 17
        Caption = 'Type &photos (catalogue pictures)'
        TabOrder = 1
      end
      object chkLoadPrices: TCheckBox
        Left = 12
        Top = 128
        Width = 508
        Height = 17
        Caption = 'Numista p&rices (estimate by condition)'
        TabOrder = 2
        OnClick = chkLoadPricesClick
      end
      object lblCurrency: TLabel
        Left = 30
        Top = 157
        Width = 48
        Height = 13
        Caption = 'C&urrency:'
        FocusControl = cbCurrency
      end
      object cbCurrency: TComboBox
        Left = 250
        Top = 154
        Width = 80
        Height = 21
        Style = csDropDownList
        TabOrder = 3
        Items.Strings = (
          'UAH'
          'USD'
          'EUR')
      end
      object lblPriceDays: TLabel
        Left = 30
        Top = 185
        Width = 160
        Height = 13
        Caption = 'Update prices older than (&days):'
        FocusControl = edPriceDays
      end
      object edPriceDays: TEdit
        Left = 250
        Top = 182
        Width = 80
        Height = 21
        NumbersOnly = True
        TabOrder = 4
      end
      object lblPricesHint: TLabel
        Left = 30
        Top = 210
        Width = 490
        Height = 30
        AutoSize = False
        Caption =
          'Loaded by the command Load Numista prices (Numista menu): 1 request p' +
          'er issue; coins of one issue cost one request.'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clGrayText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ParentFont = False
        WordWrap = True
      end
    end
    object tsUpload: TTabSheet
      Caption = 'What to upload to Numista'
      ImageIndex = 2
      object lblUploadIntro: TLabel
        Left = 12
        Top = 12
        Width = 508
        Height = 13
        AutoSize = False
        Caption = 'When items are uploaded to your Numista collection, send:'
        WordWrap = True
      end
      object lblUploadAlways: TLabel
        Left = 12
        Top = 146
        Width = 508
        Height = 30
        AutoSize = False
        Caption = 'Type, issue and quantity are always sent.'
        WordWrap = True
      end
      object lblUploadHint: TLabel
        Left = 12
        Top = 178
        Width = 508
        Height = 30
        AutoSize = False
        Caption = 'Applies to the next uploads; items already on Numista are not changed.'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clGrayText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ParentFont = False
        WordWrap = True
      end
      object chkUploadGrade: TCheckBox
        Left = 12
        Top = 38
        Width = 508
        Height = 17
        Caption = '&Condition (grade)'
        TabOrder = 0
      end
      object chkUploadPrice: TCheckBox
        Left = 12
        Top = 64
        Width = 508
        Height = 17
        Caption = 'My &price (UAH)'
        TabOrder = 1
      end
      object chkUploadDate: TCheckBox
        Left = 12
        Top = 90
        Width = 508
        Height = 17
        Caption = 'Purchase &date'
        TabOrder = 2
      end
      object chkUploadComment: TCheckBox
        Left = 12
        Top = 116
        Width = 508
        Height = 17
        Caption = 'Co&mment and swap (private comment)'
        TabOrder = 3
      end
    end
    object tsRequests: TTabSheet
      Caption = 'Requests'
      ImageIndex = 3
      object lblUsed: TLabel
        Left = 12
        Top = 14
        Width = 508
        Height = 13
        AutoSize = False
        Caption = 'Used this month: 0 of 2000'
      end
      object lblUsedNote: TLabel
        Left = 12
        Top = 34
        Width = 508
        Height = 44
        AutoSize = False
        Caption =
          'The quota is renewed on the 1st of every month. The program coun' +
          'ts only its own requests.'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clGrayText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ParentFont = False
        WordWrap = True
      end
      object lblQuota: TLabel
        Left = 12
        Top = 93
        Width = 70
        Height = 13
        Caption = 'Monthly &quota:'
        FocusControl = edQuota
      end
      object lblCounter: TLabel
        Left = 12
        Top = 125
        Width = 106
        Height = 13
        Caption = 'Requests &this month:'
        FocusControl = edCounter
      end
      object lblCounterHint: TLabel
        Left = 12
        Top = 154
        Width = 508
        Height = 44
        AutoSize = False
        Caption =
          'Correct the counter if the same key was used elsewhere (another ' +
          'computer or program).'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clGrayText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ParentFont = False
        WordWrap = True
      end
      object edQuota: TEdit
        Left = 200
        Top = 90
        Width = 80
        Height = 21
        NumbersOnly = True
        TabOrder = 0
      end
      object edCounter: TEdit
        Left = 200
        Top = 122
        Width = 80
        Height = 21
        NumbersOnly = True
        TabOrder = 1
      end
    end
  end
  object btnOK: TButton
    Left = 396
    Top = 296
    Width = 75
    Height = 25
    Caption = 'OK'
    Default = True
    TabOrder = 1
    OnClick = btnOKClick
  end
  object btnCancel: TButton
    Left = 477
    Top = 296
    Width = 75
    Height = 25
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 2
  end
end
