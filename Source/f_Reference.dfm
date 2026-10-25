object fmReference: TfmReference
  Left = 0
  Top = 0
  Caption = 'Reference'
  ClientHeight = 480
  ClientWidth = 640
  Color = clBtnFace
  Constraints.MinHeight = 400
  Constraints.MinWidth = 600
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  Position = poOwnerFormCenter
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object PageControl: TPageControl
    Left = 0
    Top = 0
    Width = 640
    Height = 443
    ActivePage = tabCountries
    Align = alClient
    TabOrder = 0
    object tabCountries: TTabSheet
      Caption = 'Countries'
      object pnlCountriesTop: TPanel
        Left = 0
        Top = 0
        Width = 632
        Height = 28
        Align = alTop
        BevelOuter = bvNone
        TabOrder = 0
        object chkOnlyNoContinent: TCheckBox
          Left = 8
          Top = 6
          Width = 300
          Height = 17
          Caption = 'Only without continent'
          TabOrder = 0
          OnClick = chkOnlyNoContinentClick
        end
      end
      object dgCountries: TDrawGrid
        Left = 0
        Top = 28
        Width = 632
        Height = 283
        Align = alClient
        ColCount = 4
        DefaultRowHeight = 18
        FixedCols = 0
        RowCount = 2
        Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goColSizing, goRowSelect, goDrawFocusSelected, goThumbTracking]
        TabOrder = 1
        OnDrawCell = GridDrawCell
        OnSelectCell = GridSelectCell
        ColWidths = (
          250
          170
          80
          80)
      end
      object pnlCountryEdit: TPanel
        Left = 0
        Top = 311
        Width = 632
        Height = 104
        Align = alBottom
        BevelOuter = bvNone
        TabOrder = 2
        object lblCountryName: TLabel
          Left = 8
          Top = 12
          Width = 31
          Height = 13
          Caption = '&Name:'
          FocusControl = edCountryName
        end
        object lblContinent: TLabel
          Left = 8
          Top = 39
          Width = 51
          Height = 13
          Caption = 'C&ontinent:'
          FocusControl = cbContinent
        end
        object lblRenameWarning: TLabel
          Left = 8
          Top = 86
          Width = 616
          Height = 13
          AutoSize = False
          Caption = 'The uCoin import matches countries by name: after renaming, the old name will be created again on import.'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clMaroon
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = []
          ParentFont = False
          Visible = False
        end
        object edCountryName: TEdit
          Left = 100
          Top = 9
          Width = 250
          Height = 21
          TabOrder = 0
          OnChange = edCountryNameChange
        end
        object cbContinent: TComboBox
          Left = 100
          Top = 36
          Width = 250
          Height = 21
          Style = csDropDownList
          TabOrder = 1
        end
        object chkExists: TCheckBox
          Left = 100
          Top = 63
          Width = 250
          Height = 17
          Caption = '&Exists (not an extinct state)'
          TabOrder = 2
        end
        object btnCountryAdd: TButton
          Left = 370
          Top = 7
          Width = 80
          Height = 25
          Caption = '&Add new'
          TabOrder = 3
          OnClick = btnCountryAddClick
        end
        object btnCountrySave: TButton
          Left = 456
          Top = 7
          Width = 80
          Height = 25
          Caption = '&Save'
          TabOrder = 4
          OnClick = btnCountrySaveClick
        end
        object btnCountryDelete: TButton
          Left = 542
          Top = 7
          Width = 80
          Height = 25
          Caption = '&Delete'
          TabOrder = 5
          OnClick = btnCountryDeleteClick
        end
      end
    end
    object tabPeriods: TTabSheet
      Caption = 'Periods'
      ImageIndex = 1
      object pnlPeriodsTop: TPanel
        Left = 0
        Top = 0
        Width = 632
        Height = 32
        Align = alTop
        BevelOuter = bvNone
        TabOrder = 0
        object lblPeriodCountry: TLabel
          Left = 8
          Top = 9
          Width = 43
          Height = 13
          Caption = 'Co&untry:'
          FocusControl = cbPeriodCountry
        end
        object cbPeriodCountry: TComboBox
          Left = 100
          Top = 6
          Width = 300
          Height = 21
          Style = csDropDownList
          DropDownCount = 25
          TabOrder = 0
          OnChange = cbPeriodCountryChange
        end
      end
      object dgPeriods: TDrawGrid
        Left = 0
        Top = 32
        Width = 632
        Height = 339
        Align = alClient
        ColCount = 2
        DefaultRowHeight = 18
        FixedCols = 0
        RowCount = 2
        Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goColSizing, goRowSelect, goDrawFocusSelected, goThumbTracking]
        TabOrder = 1
        OnDrawCell = GridDrawCell
        OnSelectCell = GridSelectCell
        ColWidths = (
          480
          80)
      end
      object pnlPeriodEdit: TPanel
        Left = 0
        Top = 371
        Width = 632
        Height = 44
        Align = alBottom
        BevelOuter = bvNone
        TabOrder = 2
        object lblPeriodName: TLabel
          Left = 8
          Top = 14
          Width = 31
          Height = 13
          Caption = 'N&ame:'
          FocusControl = edPeriodName
        end
        object edPeriodName: TEdit
          Left = 100
          Top = 11
          Width = 250
          Height = 21
          TabOrder = 0
        end
        object btnPeriodAdd: TButton
          Left = 370
          Top = 9
          Width = 80
          Height = 25
          Caption = 'Add new'
          TabOrder = 1
          OnClick = btnPeriodAddClick
        end
        object btnPeriodSave: TButton
          Left = 456
          Top = 9
          Width = 80
          Height = 25
          Caption = 'Save'
          TabOrder = 2
          OnClick = btnPeriodSaveClick
        end
        object btnPeriodDelete: TButton
          Left = 542
          Top = 9
          Width = 80
          Height = 25
          Caption = 'Delete'
          TabOrder = 3
          OnClick = btnPeriodDeleteClick
        end
      end
    end
    object tabCurrencies: TTabSheet
      Caption = 'Currencies'
      ImageIndex = 2
      object dgCurrencies: TDrawGrid
        Left = 0
        Top = 0
        Width = 632
        Height = 371
        Align = alClient
        ColCount = 2
        DefaultRowHeight = 18
        FixedCols = 0
        RowCount = 2
        Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goColSizing, goRowSelect, goDrawFocusSelected, goThumbTracking]
        TabOrder = 0
        OnDrawCell = GridDrawCell
        OnSelectCell = GridSelectCell
        ColWidths = (
          480
          80)
      end
      object pnlCurrencyEdit: TPanel
        Left = 0
        Top = 371
        Width = 632
        Height = 44
        Align = alBottom
        BevelOuter = bvNone
        TabOrder = 1
        object lblCurrencyName: TLabel
          Left = 8
          Top = 14
          Width = 31
          Height = 13
          Caption = 'Na&me:'
          FocusControl = edCurrencyName
        end
        object edCurrencyName: TEdit
          Left = 100
          Top = 11
          Width = 250
          Height = 21
          TabOrder = 0
        end
        object btnCurrencyAdd: TButton
          Left = 370
          Top = 9
          Width = 80
          Height = 25
          Caption = 'Add new'
          TabOrder = 1
          OnClick = btnCurrencyAddClick
        end
        object btnCurrencySave: TButton
          Left = 456
          Top = 9
          Width = 80
          Height = 25
          Caption = 'Save'
          TabOrder = 2
          OnClick = btnCurrencySaveClick
        end
        object btnCurrencyDelete: TButton
          Left = 542
          Top = 9
          Width = 80
          Height = 25
          Caption = 'Delete'
          TabOrder = 3
          OnClick = btnCurrencyDeleteClick
        end
      end
    end
  end
  object pnlBottom: TPanel
    Left = 0
    Top = 443
    Width = 640
    Height = 37
    Align = alBottom
    BevelOuter = bvNone
    TabOrder = 1
    DesignSize = (
      640
      37)
    object btnClose: TButton
      Left = 557
      Top = 6
      Width = 75
      Height = 25
      Anchors = [akTop, akRight]
      Cancel = True
      Caption = 'Close'
      ModalResult = 1
      TabOrder = 0
    end
  end
end
