#!/usr/bin/env python3
"""Test MCP connection to verify the integration works"""

import requests
import json

def test_mcp_endpoint():
    """Test the MCP endpoint to see what tools are available"""
    url = "http://127.0.0.1:8089/mcp"
    
    # MCP init request
    mcp_request = {
        "jsonrpc": "2.0",
        "id": 1,
        "method": "initialize",
        "params": {
            "protocolVersion": "2024-11-05",
            "capabilities": {
                "tools": {}
            },
            "clientInfo": {
                "name": "test-client",
                "version": "1.0.0"
            }
        }
    }
    
    headers = {
        "Content-Type": "application/json",
        "Accept": "application/json, text/event-stream"
    }
    
    try:
        print("Testing MCP connection...")
        response = requests.post(url, json=mcp_request, headers=headers, timeout=10)
        print(f"Status: {response.status_code}")
        print(f"Headers: {dict(response.headers)}")
        
        if response.status_code == 200:
            result = response.json()
            print("MCP Response:")
            print(json.dumps(result, indent=2))
        else:
            print(f"Error response: {response.text}")
            
    except Exception as e:
        print(f"Error: {e}")

def test_tools_list():
    """Test listing available MCP tools"""
    url = "http://127.0.0.1:8089/mcp"
    
    tools_request = {
        "jsonrpc": "2.0", 
        "id": 2,
        "method": "tools/list",
        "params": {}
    }
    
    headers = {
        "Content-Type": "application/json",
        "Accept": "application/json, text/event-stream"
    }
    
    try:
        print("\nTesting MCP tools list...")
        response = requests.post(url, json=tools_request, headers=headers, timeout=10)
        print(f"Status: {response.status_code}")
        
        if response.status_code == 200:
            result = response.json()
            print("Available MCP Tools:")
            print(json.dumps(result, indent=2))
        else:
            print(f"Error response: {response.text}")
            
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    test_mcp_endpoint()
    test_tools_list()