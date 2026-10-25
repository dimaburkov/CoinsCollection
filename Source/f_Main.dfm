object fmMain: TfmMain
  Left = 0
  Top = 0
  ActiveControl = dgItems
  Caption = 'Coins Collection'
  ClientHeight = 660
  ClientWidth = 1100
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
    object tbSep2: TToolButton
      Left = 210
      Top = 0
      Width = 16
      Style = tbsSeparator
    end
    object edSearch: TEdit
      Left = 226
      Top = 0
      Width = 220
      Height = 21
      TabOrder = 0
      TextHint = 'Search'
      OnChange = edSearchChange
      OnKeyDown = edSearchKeyDown
    end
    object tbClearSearch: TToolButton
      Left = 446
      Top = 0
      Hint = 'Clear search'
      AutoSize = True
      Caption = #215
      ParentShowHint = False
      ShowHint = True
      OnClick = tbClearSearchClick
    end
  end
  object tvNav: TTreeView
    Left = 0
    Top = 22
    Width = 240
    Height = 480
    Align = alLeft
    HideSelection = False
    Indent = 19
    ReadOnly = True
    TabOrder = 1
    OnChange = tvNavChange
  end
  object Splitter: TSplitter
    Left = 240
    Top = 22
    Width = 4
    Height = 480
    MinSize = 120
  end
  object pnlRight: TPanel
    Left = 244
    Top = 22
    Width = 856
    Height = 619
    Align = alClient
    BevelOuter = bvNone
    TabOrder = 2
    object dgItems: TDrawGrid
      Left = 0
      Top = 0
      Width = 856
      Height = 425
      Align = alClient
      ColCount = 7
      DefaultRowHeight = 18
      FixedCols = 0
      RowCount = 2
      Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goColSizing, goRowSelect, goDrawFocusSelected, goThumbTracking, goFixedRowClick, goFixedHotTrack]
      TabOrder = 0
      OnDblClick = dgItemsDblClick
      OnDrawCell = dgItemsDrawCell
      OnFixedCellClick = dgItemsFixedCellClick
      OnKeyDown = dgItemsKeyDown
      OnMouseDown = dgItemsMouseDown
      OnSelectCell = dgItemsSelectCell
      ColWidths = (
        130
        160
        120
        130
        50
        65
        85)
    end
    object splDetails: TSplitter
      Left = 0
      Top = 425
      Width = 856
      Height = 4
      Cursor = crVSplit
      Align = alBottom
      MinSize = 120
    end
    object pnlDetails: TPanel
      Left = 0
      Top = 429
      Width = 856
      Height = 190
      Align = alBottom
      BevelOuter = bvNone
      TabOrder = 1
      object pnlPhotos: TPanel
        Left = 0
        Top = 0
        Width = 344
        Height = 190
        Align = alLeft
        BevelOuter = bvNone
        TabOrder = 0
        object pnlObverse: TPanel
          Left = 8
          Top = 6
          Width = 160
          Height = 160
          BevelOuter = bvLowered
          Color = clWindow
          ParentBackground = False
          TabOrder = 0
          object imgObverse: TImage
            Left = 1
            Top = 1
            Width = 158
            Height = 158
            Align = alClient
            Center = True
            Proportional = True
            Stretch = True
          end
          object lblObverseNone: TLabel
            Left = 1
            Top = 1
            Width = 158
            Height = 158
            Align = alClient
            Alignment = taCenter
            AutoSize = False
            Caption = 'No photo'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clGrayText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = []
            ParentFont = False
            Transparent = True
            Layout = tlCenter
          end
        end
        object lblObverse: TLabel
          Left = 8
          Top = 170
          Width = 160
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'Obverse'
        end
        object pnlReverse: TPanel
          Left = 176
          Top = 6
          Width = 160
          Height = 160
          BevelOuter = bvLowered
          Color = clWindow
          ParentBackground = False
          TabOrder = 1
          object imgReverse: TImage
            Left = 1
            Top = 1
            Width = 158
            Height = 158
            Align = alClient
            Center = True
            Proportional = True
            Stretch = True
          end
          object lblReverseNone: TLabel
            Left = 1
            Top = 1
            Width = 158
            Height = 158
            Align = alClient
            Alignment = taCenter
            AutoSize = False
            Caption = 'No photo'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clGrayText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = []
            ParentFont = False
            Transparent = True
            Layout = tlCenter
          end
        end
        object lblReverse: TLabel
          Left = 176
          Top = 170
          Width = 160
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'Reverse'
        end
      end
      object sbInfo: TScrollBox
        Left = 344
        Top = 0
        Width = 512
        Height = 190
        HorzScrollBar.Visible = False
        VertScrollBar.Tracking = True
        Align = alClient
        BorderStyle = bsNone
        TabOrder = 1
        OnResize = sbInfoResize
        object lblDetailsTitle: TLabel
          Left = 8
          Top = 8
          Width = 400
          Height = 16
          AutoSize = False
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
          ShowAccelChar = False
          WordWrap = True
        end
      end
    end
  end
  object StatusBar: TStatusBar
    Left = 0
    Top = 502
    Width = 884
    Height = 19
    Panels = <
      item
        Width = 220
      end
      item
        Width = 160
      end
      item
        Width = 250
      end>
  end
  object ActionList: TActionList
    Left = 344
    Top = 96
    object actImport: TAction
      Category = 'File'
      Caption = '&Import from uCoin (.xlsx)...'
      OnExecute = actImportExecute
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
      ShortCut = 45
      OnExecute = actAddExecute
    end
    object actEdit: TAction
      Category = 'Collection'
      Caption = '&Edit'
      OnExecute = actEditExecute
      OnUpdate = actEditUpdate
    end
    object actDelete: TAction
      Category = 'Collection'
      Caption = '&Delete'
      ShortCut = 46
      OnExecute = actDeleteExecute
      OnUpdate = actDeleteUpdate
    end
    object actNeedToReplace: TAction
      Category = 'Collection'
      AutoCheck = True
      Caption = '&Need to replace'
      OnExecute = actNeedToReplaceExecute
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
      OnExecute = actReferenceExecute
    end
    object actPeriods: TAction
      Category = 'Reference'
      Caption = '&Periods...'
      OnExecute = actReferenceExecute
    end
    object actCurrencies: TAction
      Category = 'Reference'
      Caption = 'C&urrencies...'
      OnExecute = actReferenceExecute
    end
    object actNumistaSettings: TAction
      Category = 'Numista'
      Caption = '&Connect / Settings...'
      OnExecute = actNumistaSettingsExecute
    end
    object actNumistaFind: TAction
      Category = 'Numista'
      Caption = '&Find in Numista...'
      OnExecute = actNumistaFindExecute
      OnUpdate = actNumistaFindUpdate
    end
    object actNumistaMatch: TAction
      Category = 'Numista'
      Caption = '&Match by catalogue number...'
      OnExecute = actNumistaMatchExecute
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
      object miNumistaFind: TMenuItem
        Action = actNumistaFind
      end
      object miNumistaMatch: TMenuItem
        Action = actNumistaMatch
      end
      object miNumistaSep2: TMenuItem
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
