object fmNumistaSales: TfmNumistaSales
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu, biMaximize]
  Caption = 'Auction sales'
  ClientHeight = 440
  ClientWidth = 800
  Color = clBtnFace
  Constraints.MinHeight = 300
  Constraints.MinWidth = 600
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  KeyPreview = True
  Position = poOwnerFormCenter
  OnResize = FormResize
  OnShow = FormShow
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object lblCoin: TLabel
    AlignWithMargins = True
    Left = 8
    Top = 8
    Width = 784
    Height = 13
    Margins.Left = 8
    Margins.Top = 8
    Margins.Right = 8
    Margins.Bottom = 4
    Align = alTop
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
    ShowAccelChar = False
  end
  object pnlFilter: TPanel
    Left = 0
    Top = 25
    Width = 800
    Height = 28
    Align = alTop
    BevelOuter = bvNone
    TabOrder = 0
    DesignSize = (
      800
      28)
    object lblSource: TLabel
      Left = 690
      Top = 7
      Width = 100
      Height = 13
      Cursor = crHandPoint
      Alignment = taRightJustify
      Anchors = [akTop, akRight]
      AutoSize = False
      Caption = 'Source: Numista'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clHotLight
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = [fsUnderline]
      ParentFont = False
      OnClick = lblSourceClick
    end
    object rbIssue: TRadioButton
      Left = 8
      Top = 6
      Width = 130
      Height = 17
      Caption = 'My &issue'
      Checked = True
      TabOrder = 0
      TabStop = True
      OnClick = FilterClick
    end
    object rbType: TRadioButton
      Left = 144
      Top = 6
      Width = 130
      Height = 17
      Caption = 'Whole &type'
      TabOrder = 1
      OnClick = FilterClick
    end
    object chkMyGrade: TCheckBox
      Left = 290
      Top = 6
      Width = 280
      Height = 17
      Caption = 'Only my &condition'
      TabOrder = 2
      OnClick = FilterClick
    end
  end
  object lblSummary: TLabel
    AlignWithMargins = True
    Left = 8
    Top = 57
    Width = 784
    Height = 30
    Margins.Left = 8
    Margins.Top = 4
    Margins.Right = 8
    Margins.Bottom = 4
    Align = alTop
    AutoSize = False
    ShowAccelChar = False
    WordWrap = True
  end
  object dgSales: TDrawGrid
    AlignWithMargins = True
    Left = 8
    Top = 91
    Width = 784
    Height = 302
    Margins.Left = 8
    Margins.Top = 0
    Margins.Right = 8
    Margins.Bottom = 0
    Align = alClient
    ColCount = 7
    DefaultRowHeight = 18
    FixedCols = 0
    RowCount = 2
    Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goColSizing, goRowSelect, goDrawFocusSelected, goThumbTracking, goFixedRowClick]
    TabOrder = 1
    OnDblClick = dgSalesDblClick
    OnDrawCell = dgSalesDrawCell
    OnFixedCellClick = dgSalesFixedCellClick
  end
  object pnlButtons: TPanel
    Left = 0
    Top = 393
    Width = 800
    Height = 47
    Align = alBottom
    BevelOuter = bvNone
    TabOrder = 2
    DesignSize = (
      800
      47)
    object lblLoaded: TLabel
      Left = 248
      Top = 18
      Width = 440
      Height = 13
      Anchors = [akLeft, akTop, akRight]
      AutoSize = False
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clGrayText
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = []
      ParentFont = False
      ShowAccelChar = False
    end
    object btnOpenLot: TButton
      Left = 8
      Top = 12
      Width = 110
      Height = 25
      Caption = '&Open lot'
      TabOrder = 0
      OnClick = btnOpenLotClick
    end
    object btnRefresh: TButton
      Left = 124
      Top = 12
      Width = 110
      Height = 25
      Caption = '&Refresh'
      TabOrder = 1
      OnClick = btnRefreshClick
    end
    object btnClose: TButton
      Left = 708
      Top = 12
      Width = 84
      Height = 25
      Anchors = [akTop, akRight]
      Cancel = True
      Caption = 'Close'
      Default = True
      ModalResult = 2
      TabOrder = 2
    end
  end
end
