object fmCoinEdit: TfmCoinEdit
  Left = 0
  Top = 0
  BorderStyle = bsDialog
  Caption = 'Coin'
  ClientHeight = 588
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
  object lblShared: TLabel
    Left = 8
    Top = 8
    Width = 504
    Height = 13
    AutoSize = False
    Caption = 'Changes in the Coin block apply to all items of this coin.'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clMaroon
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
    Visible = False
  end
  object gbCoin: TGroupBox
    Left = 8
    Top = 26
    Width = 504
    Height = 220
    Caption = 'Coin'
    TabOrder = 0
    object lblCountry: TLabel
      Left = 12
      Top = 27
      Width = 43
      Height = 13
      Caption = '&Country:'
      FocusControl = cbCountry
    end
    object lblPeriod: TLabel
      Left = 12
      Top = 54
      Width = 36
      Height = 13
      Caption = '&Period:'
      FocusControl = cbPeriod
    end
    object lblCurrency: TLabel
      Left = 12
      Top = 81
      Width = 48
      Height = 13
      Caption = 'C&urrency:'
      FocusControl = cbCurrency
    end
    object lblDenomination: TLabel
      Left = 12
      Top = 108
      Width = 70
      Height = 13
      Caption = '&Denomination:'
      FocusControl = edDenomination
    end
    object lblYear: TLabel
      Left = 300
      Top = 108
      Width = 26
      Height = 13
      Caption = '&Year:'
      FocusControl = edYear
    end
    object lblVariety: TLabel
      Left = 12
      Top = 135
      Width = 40
      Height = 13
      Caption = '&Variety:'
      FocusControl = edVariety
    end
    object lblDiameter: TLabel
      Left = 300
      Top = 135
      Width = 67
      Height = 13
      Caption = 'D&iameter, mm:'
      FocusControl = edDiameter
    end
    object lblCatalogNumber: TLabel
      Left = 12
      Top = 162
      Width = 80
      Height = 13
      Caption = 'Catalog &number:'
      FocusControl = edCatalogNumber
    end
    object lblSubject: TLabel
      Left = 12
      Top = 189
      Width = 40
      Height = 13
      Caption = '&Subject:'
      FocusControl = edSubject
    end
    object cbCountry: TComboBox
      Left = 120
      Top = 24
      Width = 372
      Height = 21
      DropDownCount = 20
      Sorted = True
      TabOrder = 0
      OnChange = cbCountryChange
    end
    object cbPeriod: TComboBox
      Left = 120
      Top = 51
      Width = 372
      Height = 21
      DropDownCount = 20
      Sorted = True
      TabOrder = 1
    end
    object cbCurrency: TComboBox
      Left = 120
      Top = 78
      Width = 372
      Height = 21
      DropDownCount = 20
      Sorted = True
      TabOrder = 2
    end
    object edDenomination: TEdit
      Left = 120
      Top = 105
      Width = 170
      Height = 21
      TabOrder = 3
    end
    object edYear: TEdit
      Left = 390
      Top = 105
      Width = 102
      Height = 21
      MaxLength = 4
      NumbersOnly = True
      TabOrder = 4
    end
    object edVariety: TEdit
      Left = 120
      Top = 132
      Width = 170
      Height = 21
      TabOrder = 5
    end
    object edDiameter: TEdit
      Left = 390
      Top = 132
      Width = 102
      Height = 21
      TabOrder = 6
    end
    object edCatalogNumber: TEdit
      Left = 120
      Top = 159
      Width = 170
      Height = 21
      TabOrder = 7
    end
    object edSubject: TEdit
      Left = 120
      Top = 186
      Width = 372
      Height = 21
      TabOrder = 8
    end
  end
  object gbItem: TGroupBox
    Left = 8
    Top = 254
    Width = 504
    Height = 292
    Caption = 'My item'
    TabOrder = 1
    object lblCondition: TLabel
      Left = 12
      Top = 27
      Width = 50
      Height = 13
      Caption = 'C&ondition:'
      FocusControl = cbCondition
    end
    object lblQuantity: TLabel
      Left = 300
      Top = 27
      Width = 46
      Height = 13
      Caption = '&Quantity:'
      FocusControl = edQuantity
    end
    object lblCatalogValue: TLabel
      Left = 12
      Top = 54
      Width = 98
      Height = 13
      Caption = 'Catalog value, UAH:'
      FocusControl = edCatalogValue
    end
    object lblMyValue: TLabel
      Left = 300
      Top = 54
      Width = 76
      Height = 13
      Caption = 'My value, UAH:'
      FocusControl = edMyValue
    end
    object lblPurchaseDate: TLabel
      Left = 12
      Top = 81
      Width = 74
      Height = 13
      Caption = 'Purchase date:'
      FocusControl = dtPurchaseDate
    end
    object lblPublishedDate: TLabel
      Left = 300
      Top = 81
      Width = 55
      Height = 13
      Caption = 'Published:'
      FocusControl = dtPublishedDate
    end
    object lblColor: TLabel
      Left = 300
      Top = 108
      Width = 29
      Height = 13
      Caption = 'Color:'
      FocusControl = edColor
    end
    object lblGradingCompany: TLabel
      Left = 12
      Top = 135
      Width = 88
      Height = 13
      Caption = 'Grading company:'
      FocusControl = edGradingCompany
    end
    object lblGradingNumber: TLabel
      Left = 300
      Top = 135
      Width = 81
      Height = 13
      Caption = 'Grading number:'
      FocusControl = edGradingNumber
    end
    object lblGrade: TLabel
      Left = 12
      Top = 162
      Width = 33
      Height = 13
      Caption = 'Grade:'
      FocusControl = edGrade
    end
    object lblLabel: TLabel
      Left = 300
      Top = 162
      Width = 29
      Height = 13
      Caption = 'Label:'
      FocusControl = edLabel
    end
    object lblSwap: TLabel
      Left = 12
      Top = 189
      Width = 30
      Height = 13
      Caption = 'Swap:'
      FocusControl = edSwap
    end
    object lblComment: TLabel
      Left = 12
      Top = 216
      Width = 49
      Height = 13
      Caption = 'Co&mment:'
      FocusControl = mmComment
    end
    object cbCondition: TComboBox
      Left = 120
      Top = 24
      Width = 170
      Height = 21
      Style = csDropDownList
      DropDownCount = 12
      TabOrder = 0
    end
    object edQuantity: TEdit
      Left = 390
      Top = 24
      Width = 85
      Height = 21
      NumbersOnly = True
      TabOrder = 1
      Text = '1'
    end
    object udQuantity: TUpDown
      Left = 475
      Top = 24
      Width = 16
      Height = 21
      Associate = edQuantity
      Min = 1
      Max = 999
      Position = 1
      TabOrder = 2
      Thousands = False
    end
    object edCatalogValue: TEdit
      Left = 120
      Top = 51
      Width = 170
      Height = 21
      TabOrder = 3
    end
    object edMyValue: TEdit
      Left = 390
      Top = 51
      Width = 102
      Height = 21
      TabOrder = 4
    end
    object dtPurchaseDate: TDateTimePicker
      Left = 120
      Top = 78
      Width = 170
      Height = 21
      Date = 46000.000000000000000000
      Time = 46000.000000000000000000
      ShowCheckbox = True
      Checked = False
      TabOrder = 5
    end
    object dtPublishedDate: TDateTimePicker
      Left = 390
      Top = 78
      Width = 102
      Height = 21
      Date = 46000.000000000000000000
      Time = 46000.000000000000000000
      ShowCheckbox = True
      Checked = False
      TabOrder = 6
    end
    object chkNeedToReplace: TCheckBox
      Left = 120
      Top = 107
      Width = 170
      Height = 17
      Caption = 'Need to &replace'
      TabOrder = 7
    end
    object edColor: TEdit
      Left = 390
      Top = 105
      Width = 102
      Height = 21
      TabOrder = 8
    end
    object edGradingCompany: TEdit
      Left = 120
      Top = 132
      Width = 170
      Height = 21
      TabOrder = 9
    end
    object edGradingNumber: TEdit
      Left = 390
      Top = 132
      Width = 102
      Height = 21
      TabOrder = 10
    end
    object edGrade: TEdit
      Left = 120
      Top = 159
      Width = 170
      Height = 21
      TabOrder = 11
    end
    object edLabel: TEdit
      Left = 390
      Top = 159
      Width = 102
      Height = 21
      TabOrder = 12
    end
    object edSwap: TEdit
      Left = 120
      Top = 186
      Width = 372
      Height = 21
      TabOrder = 13
    end
    object mmComment: TMemo
      Left = 120
      Top = 213
      Width = 372
      Height = 66
      ScrollBars = ssVertical
      TabOrder = 14
    end
  end
  object btnOK: TButton
    Left = 356
    Top = 555
    Width = 75
    Height = 25
    Caption = 'OK'
    Default = True
    TabOrder = 2
    OnClick = btnOKClick
  end
  object btnCancel: TButton
    Left = 437
    Top = 555
    Width = 75
    Height = 25
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 3
  end
end
