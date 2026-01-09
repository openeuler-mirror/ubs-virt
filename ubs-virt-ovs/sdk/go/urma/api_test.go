package urma

import (
	"context"
	"errors"
	"strings"
	"testing"
	"ubs_virt_ovs_go_sdk/client"
	"ubs_virt_ovs_go_sdk/serde"
)

func newUrmaClientWithIPC(ipc client.Client) *urmaClient {
	return &urmaClient{ipc: ipc}
}

type mockIPCClient struct {
	CallFunc func(ctx context.Context, req *client.IpcRequest) (*client.IpcResponse, error)
}

func (m *mockIPCClient) Call(ctx context.Context, req *client.IpcRequest) (*client.IpcResponse, error) {
	if m.CallFunc != nil {
		return m.CallFunc(ctx, req)
	}
	msg, _ := serde.SerializeMsg(&client.BaseResponse{Ret: 0})
	return &client.IpcResponse{Code: 0, Payload: msg}, nil
}

func judgeCallFunc(tc struct {
	name        string
	min         uint32
	max         uint32
	expectError bool
	errContains string
}, mockIpc *mockIPCClient) {
	if tc.name == "connectFail" {
		mockIpc.CallFunc = func(ctx context.Context, req *client.IpcRequest) (*client.IpcResponse, error) {
			return nil, errors.New(tc.errContains)
		}
	} else if tc.name == "ipcFail" {
		mockIpc.CallFunc = func(ctx context.Context, req *client.IpcRequest) (*client.IpcResponse, error) {
			return &client.IpcResponse{Code: 1}, nil
		}
	} else if tc.name == "bizFail" {
		msg, _ := serde.SerializeMsg(&client.BaseResponse{Ret: 1})
		mockIpc.CallFunc = func(ctx context.Context, req *client.IpcRequest) (*client.IpcResponse, error) {
			return &client.IpcResponse{Code: 0, Payload: msg}, nil
		}
	} else {
		mockIpc.CallFunc = nil
	}
}

func TestUbsSetUrmaBandwidth(t *testing.T) {
	ctx := context.Background()
	mockIpc := &mockIPCClient{}
	newUrmaClientFunc = func() *urmaClient {
		return newUrmaClientWithIPC(mockIpc)
	}
	testCases := []struct {
		name        string
		min         uint32
		max         uint32
		expectError bool
		errContains string
	}{
		{"eth0", 1, 10, false, ""},
		{"", 1, 10, true, "name length must be"},
		{"this_name_is_way_too_long_for_urma_client", 1, 10, true, "name length must be"},
		{"eth0", 0, 10, true, "bandwidth must be between"},
		{"eth0", 1, 100, true, "bandwidth must be between"},
		{"eth0", 10, 1, true, "minBandwidth must be less than or equal to maxBandwidth"},
		{"connectFail", 1, 10, true, "connectFail"},
		{"ipcFail", 1, 10, true, "ipc {ubs.urma SetBandwidth} failed,code=1"},
		{"bizFail", 1, 10, true, "SetBandwidth bizFail failed,errCode=1"},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			judgeCallFunc(tc, mockIpc)
			err := UbsSetUrmaBandwidth(ctx, tc.name, tc.min, tc.max)
			if tc.expectError {
				if err == nil {
					t.Errorf("expect error for test case %+v,but got nil", tc)
				} else if !strings.Contains(err.Error(), tc.errContains) {
					t.Errorf("expect error to contain %q, but got %v for test case %+v", tc.errContains, err, tc)
				}
			} else {
				if err != nil {
					t.Errorf("expect no error, but got error %+v for test case %+v", err, tc)
				}
			}
		})
	}
}

func TestUbsUpdateUrmaBandwidth(t *testing.T) {
	ctx := context.Background()
	mockIpc := &mockIPCClient{}
	newUrmaClientFunc = func() *urmaClient {
		return newUrmaClientWithIPC(mockIpc)
	}
	testCases := []struct {
		name        string
		min         uint32
		max         uint32
		expectError bool
		errContains string
	}{
		{"eth0", 1, 10, false, ""},
		{"", 1, 10, true, "name length must be"},
		{"this_name_is_way_too_long_for_urma_client", 1, 10, true, "name length must be"},
		{"eth0", 0, 10, true, "bandwidth must be between"},
		{"eth0", 1, 100, true, "bandwidth must be between"},
		{"eth0", 10, 1, true, "minBandwidth must be less than or equal to maxBandwidth"},
		{"connectFail", 1, 10, true, "connectFail"},
		{"ipcFail", 1, 10, true, "ipc {ubs.urma UpdateBandwidth} failed,code=1"},
		{"bizFail", 1, 10, true, "UpdateBandwidth bizFail failed,errCode=1"},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			judgeCallFunc(tc, mockIpc)
			err := UbsUpdateUrmaBandwidth(ctx, tc.name, tc.min, tc.max)
			if tc.expectError {
				if err == nil {
					t.Errorf("expect error for test case %+v,but got nil", tc)
				} else if !strings.Contains(err.Error(), tc.errContains) {
					t.Errorf("expect error to contain %q, but got %v for test case %+v", tc.errContains, err, tc)
				}
			} else {
				if err != nil {
					t.Errorf("expect no error, but got error %+v for test case %+v", err, tc)
				}
			}
		})
	}
}

func TestUbsResetUrmaBandwidth(t *testing.T) {
	ctx := context.Background()
	mockIpc := &mockIPCClient{}
	newUrmaClientFunc = func() *urmaClient {
		return newUrmaClientWithIPC(mockIpc)
	}
	testCases := []struct {
		name        string
		min         uint32
		max         uint32
		expectError bool
		errContains string
	}{
		{"eth0", 1, 10, false, ""},
		{"", 1, 10, true, "name length must be"},
		{"this_name_is_way_too_long_for_urma_client", 1, 10, true, "name length must be"},
		{"connectFail", 1, 10, true, "connectFail"},
		{"ipcFail", 1, 10, true, "ipc {ubs.urma ResetBandwidth} failed,code=1"},
		{"bizFail", 1, 10, true, "resetBandwidth bizFail failed,errCode=1"},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			judgeCallFunc(tc, mockIpc)
			err := UbsResetUrmaBandwidth(ctx, tc.name)
			if tc.expectError {
				if err == nil {
					t.Errorf("expect error for test case %+v,but got nil", tc)
				} else if !strings.Contains(err.Error(), tc.errContains) {
					t.Errorf("expect error to contain %q, but got %v for test case %+v", tc.errContains, err, tc)
				}
			} else {
				if err != nil {
					t.Errorf("expect no error, but got error %+v for test case %+v", err, tc)
				}
			}
		})
	}
}
