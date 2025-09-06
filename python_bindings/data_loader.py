#!/usr/bin/env python3
"""
Data Loader for Backtrader C++ Python Bindings Tests
从原版backtrader测试数据中加载数据，确保与原版测试一致
"""

import csv
import os
from typing import List, Dict, Any


class BacktraderTestData:
    """加载和解析原版backtrader测试数据"""
    
    def __init__(self):
        # 原版测试数据路径
        self.data_path = "/home/yun/Documents/refactor_backtrader/backtrader/tests/datas"
        self.data_files = [
            '2006-day-001.txt',
            '2006-week-001.txt',
        ]
    
    def load_csv_data(self, file_index=0) -> List[Dict[str, float]]:
        """
        加载CSV数据，与原版testcommon.getdata()相同
        返回OHLCV数据列表
        """
        if file_index >= len(self.data_files):
            raise ValueError(f"Invalid file index: {file_index}")
        
        file_path = os.path.join(self.data_path, self.data_files[file_index])
        
        if not os.path.exists(file_path):
            raise FileNotFoundError(f"Test data file not found: {file_path}")
        
        data = []
        with open(file_path, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                # 转换为浮点数，与原版backtrader数据格式一致
                data_point = {
                    'date': row['Date'],
                    'open': float(row['Open']),
                    'high': float(row['High']),
                    'low': float(row['Low']),
                    'close': float(row['Close']),
                    'volume': float(row['Volume']),
                    'openinterest': float(row['OpenInterest'])
                }
                data.append(data_point)
        
        return data
    
    def get_close_prices(self, file_index=0) -> List[float]:
        """
        获取收盘价序列，用于技术指标计算
        这与原版backtrader测试中使用的数据相同
        """
        data = self.load_csv_data(file_index)
        return [point['close'] for point in data]
    
    def get_ohlc_data(self, file_index=0) -> Dict[str, List[float]]:
        """
        获取OHLC数据，用于更复杂的技术指标
        """
        data = self.load_csv_data(file_index)
        return {
            'open': [point['open'] for point in data],
            'high': [point['high'] for point in data],
            'low': [point['low'] for point in data],
            'close': [point['close'] for point in data],
            'volume': [point['volume'] for point in data]
        }
    
    def get_data_info(self, file_index=0) -> Dict[str, Any]:
        """获取数据基本信息"""
        data = self.load_csv_data(file_index)
        close_prices = [point['close'] for point in data]
        
        return {
            'size': len(data),
            'start_date': data[0]['date'],
            'end_date': data[-1]['date'],
            'price_range': {
                'min': min(close_prices),
                'max': max(close_prices),
                'start': close_prices[0],
                'end': close_prices[-1]
            },
            'file_name': self.data_files[file_index]
        }


class OriginalTestParameters:
    """原版backtrader测试参数"""
    
    # SMA测试参数 (来自 test_ind_sma.py)
    SMA_TEST = {
        'min_period': 30,
        'expected_values': ['4063.463000', '3644.444667', '3554.693333'],
        'data_files': 1  # 使用第一个数据文件
    }
    
    # EMA测试参数 (来自 test_ind_ema.py)
    EMA_TEST = {
        'min_period': 30,
        'expected_values': ['4070.115719', '3644.444667', '3581.728712'],
        'data_files': 1
    }
    
    # RSI测试参数 (来自 test_ind_rsi.py)
    RSI_TEST = {
        'min_period': 15,  # RSI实际上是14周期，但测试中chkmin=15
        'expected_values': ['57.644284', '41.630968', '53.352553'],
        'data_files': 1
    }
    
    @classmethod
    def get_check_points(cls, data_length: int, min_period: int) -> List[int]:
        """
        计算检查点，与原版testcommon.py中的算法一致
        chkpts = [0, -l + mp, (-l + mp) // 2]
        """
        l = data_length
        mp = min_period
        return [0, -l + mp, (-l + mp) // 2]
    
    @classmethod
    def get_expected_values_at_checkpoints(cls, test_name: str) -> List[str]:
        """获取指定测试的期望值"""
        test_params = {
            'SMA': cls.SMA_TEST,
            'EMA': cls.EMA_TEST,
            'RSI': cls.RSI_TEST
        }
        
        if test_name not in test_params:
            raise ValueError(f"Unknown test: {test_name}")
        
        return test_params[test_name]['expected_values']


def test_data_loader():
    """测试数据加载器"""
    print("=== 测试数据加载器 ===")
    
    loader = BacktraderTestData()
    
    # 测试数据加载
    print("1. 加载数据...")
    data_info = loader.get_data_info(0)
    print(f"   数据文件: {data_info['file_name']}")
    print(f"   数据大小: {data_info['size']}")
    print(f"   日期范围: {data_info['start_date']} 到 {data_info['end_date']}")
    print(f"   价格范围: {data_info['price_range']['min']:.2f} - {data_info['price_range']['max']:.2f}")
    
    # 测试收盘价提取
    close_prices = loader.get_close_prices(0)
    print(f"\n2. 收盘价数据:")
    print(f"   总数量: {len(close_prices)}")
    print(f"   前5个: {close_prices[:5]}")
    print(f"   后5个: {close_prices[-5:]}")
    
    # 测试检查点计算
    print(f"\n3. 检查点计算:")
    for test_name in ['SMA', 'EMA', 'RSI']:
        if test_name == 'SMA':
            params = OriginalTestParameters.SMA_TEST
        elif test_name == 'EMA':
            params = OriginalTestParameters.EMA_TEST
        else:
            params = OriginalTestParameters.RSI_TEST
        
        check_points = OriginalTestParameters.get_check_points(
            len(close_prices), params['min_period']
        )
        expected = OriginalTestParameters.get_expected_values_at_checkpoints(test_name)
        
        print(f"   {test_name}: 检查点 {check_points}, 期望值 {expected}")
    
    print("\n✅ 数据加载器测试完成")
    return True


if __name__ == "__main__":
    test_data_loader()