import requests

testHost = 'http://localhost:8080'
# 发送GET请求
def test_get_request():
    response = requests.get(testHost)
    assert response.status_code == 200
    # assert 'content_type' in response.headers
    # assert 'application/json' in response.headers['content_type']
    # 这里可以添加更多的断言，验证响应的内容等

# 发送POST请求
def test_post_request():
    payload = {'key1': 'value1', 'key2': 'value2'}
    response = requests.post('http://yourserver.com/api', json=payload)
    assert response.status_code == 201  # 假设创建成功返回状态码是201
    assert 'location' in response.headers
    # 这里可以添加更多的断言，验证创建资源的位置等

# 发送PUT请求
def test_put_request():
    payload = {'key1': 'updated_value1', 'key2': 'updated_value2'}
    response = requests.put('http://yourserver.com/api/1', json=payload)
    assert response.status_code == 200
    # 这里可以添加更多的断言，验证更新操作的结果等

# 发送DELETE请求
def test_delete_request():
    response = requests.delete('http://yourserver.com/api/1')
    assert response.status_code == 204  # 假设删除成功返回状态码是204
    # 这里可以添加更多的断言，验证资源是否被成功删除等

# 可以继续编写测试函数来测试其他类型的HTTP请求，如PATCH、OPTIONS等。

if __name__ == "__main__":
    test_get_request()
    # test_post_request()
    # test_put_request()
    # test_delete_request()
    print("All tests passed successfully!")
